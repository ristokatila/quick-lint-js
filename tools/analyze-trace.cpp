// Copyright (C) 2020  Matthew "strager" Glazar
// See end of file for extended copyright information.

#include <cerrno>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <quick-lint-js/cli/arg-parser.h>
#include <quick-lint-js/container/hash-map.h>
#include <quick-lint-js/container/padded-string.h>
#include <quick-lint-js/document.h>
#include <quick-lint-js/io/file-path.h>
#include <quick-lint-js/io/file.h>
#include <quick-lint-js/io/temporary-directory.h>
#include <quick-lint-js/logging/trace-stream-reader.h>
#include <quick-lint-js/lsp/lsp-location.h>
#include <quick-lint-js/port/char8.h>
#include <quick-lint-js/port/integer.h>
#include <quick-lint-js/port/warning.h>
#include <quick-lint-js/util/narrow-cast.h>
#include <quick-lint-js/util/utf-16.h>
#include <string>
#include <string_view>
#include <vector>

QLJS_WARNING_IGNORE_GCC("-Wshadow=local")

using namespace std::literals::string_view_literals;

namespace quick_lint_js {
namespace {
struct analyze_options {
  std::vector<const char*> trace_files;
  std::optional<std::uint64_t> dump_document_content_document_id;
  bool check_document_consistency = false;
  std::uint64_t begin_event_index = 0;
  std::uint64_t end_event_index = (std::numeric_limits<std::uint64_t>::max)();
};

lsp_range to_lsp_range(
    const trace_stream_event_visitor::vscode_document_range& range) {
  return lsp_range{
      .start =
          {
              .line = narrow_cast<int>(range.start.line),
              .character = narrow_cast<int>(range.start.character),
          },
      .end =
          {
              .line = narrow_cast<int>(range.end.line),
              .character = narrow_cast<int>(range.end.character),
          },
  };
}

class counting_trace_stream_event_visitor : public trace_stream_event_visitor {
 public:
  // The first call to a visit_ function will set this to 0.
  std::uint64_t event_index = (std::numeric_limits<std::uint64_t>::max)();

  void visit_init_event(const init_event&) override { ++this->event_index; }

  void visit_vscode_document_opened_event(
      const vscode_document_opened_event&) override {
    ++this->event_index;
  }

  void visit_vscode_document_closed_event(
      const vscode_document_closed_event&) override {
    ++this->event_index;
  }

  void visit_vscode_document_changed_event(
      const vscode_document_changed_event&) override {
    ++this->event_index;
  }

  void visit_vscode_document_sync_event(
      const vscode_document_sync_event&) override {
    ++this->event_index;
  }

  void visit_lsp_client_to_server_message_event(
      const lsp_client_to_server_message_event&) override {
    ++this->event_index;
  }
};

class document_content_dumper : public counting_trace_stream_event_visitor {
 public:
  using base = counting_trace_stream_event_visitor;

  explicit document_content_dumper(std::uint64_t document_id,
                                   std::uint64_t end_event_index)
      : document_id_(document_id), end_event_index_(end_event_index) {}

  void visit_error_invalid_magic() override {
    std::fprintf(stderr, "error: invalid magic\n");
  }

  void visit_error_invalid_uuid() override {
    std::fprintf(stderr, "error: invalid UUID\n");
  }

  void visit_error_unsupported_compression_mode(std::uint8_t mode) override {
    std::fprintf(stderr, "error: unsupported compression mode: %#02x\n", mode);
  }

  void visit_packet_header(const packet_header&) override {}

  void visit_vscode_document_opened_event(
      const vscode_document_opened_event& event) override {
    base::visit_vscode_document_opened_event(event);
    if (!this->should_analyze()) return;
    if (event.document_id != this->document_id_) {
      return;
    }

    this->doc_.set_text(utf_16_to_utf_8(event.content));
  }

  void visit_vscode_document_closed_event(
      const vscode_document_closed_event& event) override {
    base::visit_vscode_document_closed_event(event);
    if (!this->should_analyze()) return;
    if (event.document_id != this->document_id_) {
      return;
    }

    this->doc_.set_text(u8"(document closed)");
  }

  void visit_vscode_document_changed_event(
      const vscode_document_changed_event& event) override {
    base::visit_vscode_document_changed_event(event);
    if (!this->should_analyze()) return;
    if (event.document_id != this->document_id_) {
      return;
    }

    for (const auto& change : event.changes) {
      this->doc_.replace_text(to_lsp_range(change.range),
                              utf_16_to_utf_8(change.text));
    }
  }

  void print_document_content() {
    padded_string_view s = this->doc_.string();
    std::fwrite(s.data(), 1, narrow_cast<std::size_t>(s.size()), stdout);
  }

 private:
  bool should_analyze() const noexcept {
    return this->event_index <= this->end_event_index_;
  }

  document<lsp_locator> doc_;
  std::uint64_t document_id_;
  std::uint64_t end_event_index_;
};

class document_content_checker : public counting_trace_stream_event_visitor {
 public:
  using base = counting_trace_stream_event_visitor;

  void visit_error_invalid_magic() override {
    std::fprintf(stderr, "error: invalid magic\n");
  }

  void visit_error_invalid_uuid() override {
    std::fprintf(stderr, "error: invalid UUID\n");
  }

  void visit_error_unsupported_compression_mode(std::uint8_t mode) override {
    std::fprintf(stderr, "error: unsupported compression mode: %#02x\n", mode);
  }

  void visit_packet_header(const packet_header&) override {}

  void visit_vscode_document_opened_event(
      const vscode_document_opened_event& event) override {
    base::visit_vscode_document_opened_event(event);
    if (event.document_id == 0) {
      return;
    }
    document_info& doc = this->documents_[event.document_id];
    doc.data.set_text(utf_16_to_utf_8(event.content));
    doc.data.locator().validate_caches_debug();
    doc.last_sync = this->event_index;
  }

  void visit_vscode_document_closed_event(
      const vscode_document_closed_event& event) override {
    base::visit_vscode_document_closed_event(event);
    if (event.document_id == 0) {
      return;
    }
    this->documents_.erase(event.document_id);
  }

  void visit_vscode_document_changed_event(
      const vscode_document_changed_event& event) override {
    base::visit_vscode_document_changed_event(event);
    if (event.document_id == 0) {
      return;
    }

    auto doc_it = this->documents_.find(event.document_id);
    if (doc_it == this->documents_.end()) {
      std::fprintf(stderr,
                   "warning: document %#llx changed but wasn't opened\n",
                   narrow_cast<unsigned long long>(event.document_id));
      return;
    }
    document_info& doc = doc_it->second;
    for (const auto& change : event.changes) {
      doc.data.replace_text(to_lsp_range(change.range),
                            utf_16_to_utf_8(change.text));
      doc.data.locator().validate_caches_debug();
    }
  }

  void visit_vscode_document_sync_event(
      const vscode_document_sync_event& event) override {
    base::visit_vscode_document_sync_event(event);
    if (event.document_id == 0) {
      return;
    }

    auto doc_it = this->documents_.find(event.document_id);
    if (doc_it == this->documents_.end()) {
      std::fprintf(stderr, "warning: document %#llx synced but wasn't opened\n",
                   narrow_cast<unsigned long long>(event.document_id));
      return;
    }
    document_info& doc = doc_it->second;

    string8_view actual = doc.data.string().string_view();
    string8 expected = utf_16_to_utf_8(event.content);
    if (actual != expected) {
      std::fprintf(stderr,
                   "error: document mismatch detected at event %" PRIu64
                   " (last sync at event %" PRIu64 ")\n",
                   this->event_index, doc.last_sync);

      std::string dir = make_temporary_directory();
      std::string expected_path =
          dir + QLJS_PREFERRED_PATH_DIRECTORY_SEPARATOR + "expected";
      write_file_or_exit(expected_path, expected);
      std::fprintf(stderr, "note: expected written to %s\n",
                   expected_path.c_str());

      std::string actual_path =
          dir + QLJS_PREFERRED_PATH_DIRECTORY_SEPARATOR + "actual";
      write_file_or_exit(actual_path, actual);
      std::fprintf(stderr, "note: actual written to %s\n", actual_path.c_str());
    }

    doc.last_sync = this->event_index;
  }

 private:
  struct document_info {
    std::uint64_t last_sync = 0;
    document<lsp_locator> data;
  };

  hash_map<std::uint64_t, document_info> documents_;
};

class event_dumper : public counting_trace_stream_event_visitor {
 private:
  static constexpr int header_width = 16;

 public:
  using base = counting_trace_stream_event_visitor;

  explicit event_dumper(std::uint64_t begin_event_index,
                        std::uint64_t end_event_index)
      : begin_event_index_(begin_event_index),
        end_event_index_(end_event_index) {}

  void visit_error_invalid_magic() override {
    std::printf("error: invalid magic\n");
  }

  void visit_error_invalid_uuid() override {
    std::printf("error: invalid UUID\n");
  }

  void visit_error_unsupported_compression_mode(std::uint8_t mode) override {
    std::printf("error: unsupported compression mode: %#02x\n", mode);
  }

  void visit_packet_header(const packet_header&) override {}

  void visit_init_event(const init_event& event) override {
    base::visit_init_event(event);
    if (!this->should_dump()) return;

    this->print_event_header(event);
    std::printf("init version='%s'\n", event.version);
  }

  void visit_vscode_document_opened_event(
      const vscode_document_opened_event& event) override {
    base::visit_vscode_document_opened_event(event);
    if (!this->should_dump()) return;

    this->print_event_header(event);
    std::printf("document ");
    this->print_document_id(event.document_id);
    std::printf(" opened: ");
    this->print_utf16(event.uri);
    std::printf("\n");
  }

  void visit_vscode_document_closed_event(
      const vscode_document_closed_event& event) override {
    base::visit_vscode_document_closed_event(event);
    if (!this->should_dump()) return;

    this->print_event_header(event);
    std::printf("document ");
    this->print_document_id(event.document_id);
    std::printf(" closed: ");
    this->print_utf16(event.uri);
    std::printf("\n");
  }

  void visit_vscode_document_changed_event(
      const vscode_document_changed_event& event) override {
    base::visit_vscode_document_changed_event(event);
    if (!this->should_dump()) return;

    this->print_event_header(event);
    std::printf("document ");
    this->print_document_id(event.document_id);
    std::printf(" changed\n");
    for (const auto& change : event.changes) {
      std::printf("%*s%llu:%llu->%llu:%llu: '", this->header_width, "",
                  narrow_cast<unsigned long long>(change.range.start.line),
                  narrow_cast<unsigned long long>(change.range.start.character),
                  narrow_cast<unsigned long long>(change.range.end.line),
                  narrow_cast<unsigned long long>(change.range.end.character));
      this->print_utf16(change.text);
      std::printf("'\n");
    }
  }

  void visit_vscode_document_sync_event(
      const vscode_document_sync_event& event) override {
    base::visit_vscode_document_sync_event(event);
    if (!this->should_dump()) return;

    this->print_event_header(event);
    std::printf("document ");
    this->print_document_id(event.document_id);
    std::printf(" sync: ");
    this->print_utf16(event.uri);
    std::printf("\n");
  }

  void visit_lsp_client_to_server_message_event(
      const lsp_client_to_server_message_event& event) override {
    base::visit_lsp_client_to_server_message_event(event);
    if (!this->should_dump()) return;

    this->print_event_header(event);
    std::printf("client->server LSP message: ");
    this->print_utf8(event.body);
    std::printf("\n");
  }

 private:
  template <class Event>
  void print_event_header(const Event& event) {
    std::uint64_t ns_per_s = 1'000'000'000;
    std::printf("@%0*llu.%09llu ", this->header_width - 1 - 1 - 9 - 1,
                narrow_cast<unsigned long long>(event.timestamp % ns_per_s),
                narrow_cast<unsigned long long>(event.timestamp / ns_per_s));
  }

  void print_document_id(std::uint64_t document_id) {
    std::printf("%#llx", narrow_cast<unsigned long long>(document_id));
  }

  void print_utf16(std::u16string_view s) {
    this->print_utf8(utf_16_to_utf_8(s));
  }

  void print_utf8(string8_view s) {
    std::fwrite(s.data(), 1, s.size(), stdout);
  }

  // Call this function after calling base::*.
  bool should_dump() const noexcept {
    return this->begin_event_index_ <= this->event_index &&
           this->event_index <= this->end_event_index_;
  }

  std::uint64_t begin_event_index_;
  std::uint64_t end_event_index_;
};

analyze_options parse_analyze_options(int argc, char** argv) {
  analyze_options o;

  arg_parser parser(argc, argv);
  while (!parser.done()) {
    if (const char* argument = parser.match_argument()) {
      o.trace_files.push_back(argument);
    } else if (parser.match_flag_option("--check-document-consistency"sv,
                                        "--check"sv)) {
      o.check_document_consistency = true;
    } else if (const char* arg_value = parser.match_option_with_value(
                   "--dump-document-content"sv)) {
      errno = 0;
      char* end;
      unsigned long long document_id =
          std::strtoull(arg_value, &end, /*base=*/0);
      if (errno != 0 || end == arg_value || *end != '\0') {
        std::fprintf(stderr, "error: malformed document ID: %s\n", arg_value);
        std::exit(2);
      }
      o.dump_document_content_document_id =
          narrow_cast<std::uint64_t>(document_id);
    } else if (const char* arg_value =
                   parser.match_option_with_value("--begin"sv)) {
      from_chars_result result =
          from_chars(&arg_value[0], &arg_value[std::strlen(arg_value)],
                     o.begin_event_index);
      if (*result.ptr != '\0' || result.ec != std::errc{}) {
        std::fprintf(stderr, "error: unrecognized option: %s\n", arg_value);
        std::exit(2);
      }
    } else if (const char* arg_value =
                   parser.match_option_with_value("--end"sv)) {
      from_chars_result result = from_chars(
          &arg_value[0], &arg_value[std::strlen(arg_value)], o.end_event_index);
      if (*result.ptr != '\0' || result.ec != std::errc{}) {
        std::fprintf(stderr, "error: unrecognized option: %s\n", arg_value);
        std::exit(2);
      }
    } else {
      const char* unrecognized = parser.match_anything();
      std::fprintf(stderr, "error: unrecognized option: %s\n", unrecognized);
      std::exit(2);
    }
  }

  return o;
}
}
}

int main(int argc, char** argv) {
  using namespace quick_lint_js;

  analyze_options o = parse_analyze_options(argc, argv);
  if (o.trace_files.empty()) {
    std::fprintf(stderr, "error: missing trace file\n");
    return 2;
  }
  if (o.trace_files.size() > 1) {
    std::fprintf(stderr, "error: unexpected arguments\n");
    return 2;
  }

  auto file = read_file(o.trace_files[0]);
  if (!file.ok()) {
    file.error().print_and_exit();
  }

  if (o.check_document_consistency) {
    document_content_checker checker;
    read_trace_stream(file->data(), narrow_cast<std::size_t>(file->size()),
                      checker);
  }

  if (o.dump_document_content_document_id.has_value()) {
    document_content_dumper dumper(*o.dump_document_content_document_id,
                                   o.end_event_index);
    read_trace_stream(file->data(), narrow_cast<std::size_t>(file->size()),
                      dumper);
    dumper.print_document_content();
  } else if (!o.check_document_consistency) {
    event_dumper dumper(o.begin_event_index, o.end_event_index);
    read_trace_stream(file->data(), narrow_cast<std::size_t>(file->size()),
                      dumper);
  }

  return 0;
}

// quick-lint-js finds bugs in JavaScript programs.
// Copyright (C) 2020  Matthew "strager" Glazar
//
// This file is part of quick-lint-js.
//
// quick-lint-js is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// quick-lint-js is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with quick-lint-js.  If not, see <https://www.gnu.org/licenses/>.
