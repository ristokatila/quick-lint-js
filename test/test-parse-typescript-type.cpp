// Copyright (C) 2020  Matthew "strager" Glazar
// See end of file for extended copyright information.

#include <algorithm>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iterator>
#include <quick-lint-js/array.h>
#include <quick-lint-js/char8.h>
#include <quick-lint-js/cli-location.h>
#include <quick-lint-js/diag-collector.h>
#include <quick-lint-js/diag-matcher.h>
#include <quick-lint-js/diagnostic-types.h>
#include <quick-lint-js/language.h>
#include <quick-lint-js/padded-string.h>
#include <quick-lint-js/parse-support.h>
#include <quick-lint-js/parse.h>
#include <quick-lint-js/spy-visitor.h>
#include <quick-lint-js/string-view.h>
#include <string>
#include <string_view>
#include <vector>

using ::testing::ElementsAre;
using ::testing::IsEmpty;

namespace quick_lint_js {
namespace {
TEST(test_parse_typescript_type, direct_type_reference) {
  {
    spy_visitor v = parse_and_visit_typescript_type(u8"Type"_sv);
    EXPECT_THAT(v.visits, ElementsAre("visit_variable_type_use"));  // Type
    EXPECT_THAT(v.variable_uses,
                ElementsAre(spy_visitor::visited_variable_use{u8"Type"}));
  }
}

TEST(test_parse_typescript_type, namespaced_type_reference) {
  {
    spy_visitor v = parse_and_visit_typescript_type(u8"ns.Type"_sv);
    EXPECT_THAT(v.visits, ElementsAre("visit_variable_namespace_use"));  // ns
    EXPECT_THAT(v.variable_uses,
                ElementsAre(spy_visitor::visited_variable_use{u8"ns"}));
  }
}

TEST(test_parse_typescript_type, builtin_types) {
  for (string8 type : {
           u8"bigint",
           u8"boolean",
           u8"null",
           u8"number",
           u8"object",
           u8"string",
           u8"symbol",
           u8"undefined",
           u8"void",
       }) {
    SCOPED_TRACE(out_string8(type));
    spy_visitor v = parse_and_visit_typescript_type(type);
    EXPECT_THAT(v.visits, IsEmpty());
    EXPECT_THAT(v.variable_uses, IsEmpty())
        << "builtin type should not be treated as a variable";
  }
}

TEST(test_parse_typescript_type, special_types) {
  for (string8 type : {
           u8"any",
           u8"never",
           u8"unknown",
       }) {
    SCOPED_TRACE(out_string8(type));
    spy_visitor v = parse_and_visit_typescript_type(type);
    EXPECT_THAT(v.visits, IsEmpty());
    EXPECT_THAT(v.variable_uses, IsEmpty())
        << "special type should not be treated as a variable";
  }
}

TEST(test_parse_typescript_type, this_type) {
  {
    spy_visitor v = parse_and_visit_typescript_type(u8"this"_sv);
    // TODO(strager): Report an error if the 'this' type is used where 'this'
    // isn't allowed. Should we visit so the linter can report errors? Or should
    // the parser keep track of whether 'this' is permitted?
    EXPECT_THAT(v.visits, IsEmpty());
    EXPECT_THAT(v.variable_uses, IsEmpty());
  }
}

TEST(test_parse_typescript_type, literal_type) {
  {
    spy_visitor v = parse_and_visit_typescript_type(u8"42"_sv);
    EXPECT_THAT(v.visits, IsEmpty());
    EXPECT_THAT(v.variable_uses, IsEmpty());
  }

  {
    spy_visitor v = parse_and_visit_typescript_type(u8"'hello'"_sv);
    EXPECT_THAT(v.visits, IsEmpty());
    EXPECT_THAT(v.variable_uses, IsEmpty());
  }
}

TEST(test_parse_typescript_type, template_literal_type) {
  {
    spy_visitor v = parse_and_visit_typescript_type(u8"`hello`"_sv);
    EXPECT_THAT(v.visits, IsEmpty());
  }

  {
    spy_visitor v = parse_and_visit_typescript_type(u8"`hello${other}`"_sv);
    EXPECT_THAT(v.visits, ElementsAre("visit_variable_type_use"));  // other
    EXPECT_THAT(v.variable_uses,
                ElementsAre(spy_visitor::visited_variable_use{u8"other"}));
  }

  {
    spy_visitor v =
        parse_and_visit_typescript_type(u8"`hello${other}${another}`"_sv);
    EXPECT_THAT(v.visits, ElementsAre("visit_variable_type_use",    // other
                                      "visit_variable_type_use"));  // another
    EXPECT_THAT(v.variable_uses,
                ElementsAre(spy_visitor::visited_variable_use{u8"other"},
                            spy_visitor::visited_variable_use{u8"another"}));
  }
}

TEST(test_parse_typescript_type, parenthesized_type) {
  {
    spy_visitor v = parse_and_visit_typescript_type(u8"(Type)"_sv);
    EXPECT_THAT(v.visits, ElementsAre("visit_variable_type_use"));  // Type
    EXPECT_THAT(v.variable_uses,
                ElementsAre(spy_visitor::visited_variable_use{u8"Type"}));
  }

  {
    spy_visitor v = parse_and_visit_typescript_type(u8"(((((Type)))))"_sv);
    EXPECT_THAT(v.visits, ElementsAre("visit_variable_type_use"));  // Type
    EXPECT_THAT(v.variable_uses,
                ElementsAre(spy_visitor::visited_variable_use{u8"Type"}));
  }
}

TEST(test_parse_typescript_type, tuple_type) {
  {
    spy_visitor v = parse_and_visit_typescript_type(u8"[]"_sv);
    EXPECT_THAT(v.visits, IsEmpty());
    EXPECT_THAT(v.variable_uses, IsEmpty());
  }

  {
    spy_visitor v = parse_and_visit_typescript_type(u8"[A]"_sv);
    EXPECT_THAT(v.visits, ElementsAre("visit_variable_type_use"));  // A
    EXPECT_THAT(v.variable_uses,
                ElementsAre(spy_visitor::visited_variable_use{u8"A"}));
  }

  {
    spy_visitor v = parse_and_visit_typescript_type(u8"[A, B, C]"_sv);
    EXPECT_THAT(v.variable_uses,
                ElementsAre(spy_visitor::visited_variable_use{u8"A"},
                            spy_visitor::visited_variable_use{u8"B"},
                            spy_visitor::visited_variable_use{u8"C"}));
  }

  {
    spy_visitor v = parse_and_visit_typescript_type(u8"[A, B, C, ]"_sv);
    EXPECT_THAT(v.variable_uses,
                ElementsAre(spy_visitor::visited_variable_use{u8"A"},
                            spy_visitor::visited_variable_use{u8"B"},
                            spy_visitor::visited_variable_use{u8"C"}));
  }
}

TEST(test_parse_typescript_type, empty_object_type) {
  spy_visitor v = parse_and_visit_typescript_type(u8"{}"_sv);
  EXPECT_THAT(v.visits, IsEmpty());
  EXPECT_THAT(v.variable_uses, IsEmpty());
}

TEST(test_parse_typescript_type, object_type_with_basic_properties) {
  {
    spy_visitor v = parse_and_visit_typescript_type(u8"{ untypedProperty }"_sv);
    EXPECT_THAT(v.visits, IsEmpty());
  }

  {
    spy_visitor v = parse_and_visit_typescript_type(u8"{ property: Type }"_sv);
    EXPECT_THAT(v.visits, ElementsAre("visit_variable_type_use"));  // Type
    EXPECT_THAT(v.variable_uses,
                ElementsAre(spy_visitor::visited_variable_use{u8"Type"}));
  }

  {
    spy_visitor v = parse_and_visit_typescript_type(u8"{ property: Type, }"_sv);
    EXPECT_THAT(v.visits, ElementsAre("visit_variable_type_use"));  // Type
  }

  {
    spy_visitor v = parse_and_visit_typescript_type(u8"{ property: Type; }"_sv);
    EXPECT_THAT(v.visits, ElementsAre("visit_variable_type_use"));  // Type
  }

  {
    spy_visitor v =
        parse_and_visit_typescript_type(u8"{ p1: Type1, p2: Type2 }"_sv);
    EXPECT_THAT(v.visits, ElementsAre("visit_variable_type_use",    // Type1
                                      "visit_variable_type_use"));  // Type2
    EXPECT_THAT(v.variable_uses,
                ElementsAre(spy_visitor::visited_variable_use{u8"Type1"},
                            spy_visitor::visited_variable_use{u8"Type2"}));
  }

  {
    spy_visitor v =
        parse_and_visit_typescript_type(u8"{ p1: Type1; p2: Type2 }"_sv);
    EXPECT_THAT(v.visits, ElementsAre("visit_variable_type_use",    // Type1
                                      "visit_variable_type_use"));  // Type2
    EXPECT_THAT(v.variable_uses,
                ElementsAre(spy_visitor::visited_variable_use{u8"Type1"},
                            spy_visitor::visited_variable_use{u8"Type2"}));
  }
}

TEST(test_parse_typescript_type, object_type_allows_asi_between_properties) {
  {
    spy_visitor v =
        parse_and_visit_typescript_type(u8"{\n  p1: Type1\n  p2: Type2\n}"_sv);
    EXPECT_THAT(v.visits, ElementsAre("visit_variable_type_use",    // Type1
                                      "visit_variable_type_use"));  // Type2
    EXPECT_THAT(v.variable_uses,
                ElementsAre(spy_visitor::visited_variable_use{u8"Type1"},
                            spy_visitor::visited_variable_use{u8"Type2"}));
  }
}

TEST(test_parse_typescript_type,
     object_type_requires_separator_between_properties) {
  {
    padded_string code(u8"{ p1: Type1 p2: Type2 }"_sv);
    spy_visitor v;
    parser p(&code, &v, typescript_options);
    p.parse_and_visit_typescript_type_expression(v);
    EXPECT_THAT(v.visits, ElementsAre("visit_variable_type_use",    // Type1
                                      "visit_variable_type_use"));  // Type2
    EXPECT_THAT(v.errors,
                ElementsAre(DIAG_TYPE_OFFSETS(
                    &code, diag_missing_separator_between_object_type_entries,
                    expected_separator, strlen(u8"{ p1: Type1"), u8"")));
  }
}

TEST(test_parse_typescript_type, object_type_with_readonly_properties) {
  {
    spy_visitor v =
        parse_and_visit_typescript_type(u8"{ readonly untypedProperty }"_sv);
    EXPECT_THAT(v.visits, IsEmpty());
  }

  {
    spy_visitor v =
        parse_and_visit_typescript_type(u8"{ readonly property: Type }"_sv);
    EXPECT_THAT(v.visits, ElementsAre("visit_variable_type_use"));  // Type
    EXPECT_THAT(v.variable_uses,
                ElementsAre(spy_visitor::visited_variable_use{u8"Type"}));
  }
}

TEST(test_parse_typescript_type, object_type_with_optional_properties) {
  {
    spy_visitor v =
        parse_and_visit_typescript_type(u8"{ untypedProperty? }"_sv);
    EXPECT_THAT(v.visits, IsEmpty());
  }

  {
    spy_visitor v = parse_and_visit_typescript_type(u8"{ property?: Type }"_sv);
    EXPECT_THAT(v.visits, ElementsAre("visit_variable_type_use"));  // Type
    EXPECT_THAT(v.variable_uses,
                ElementsAre(spy_visitor::visited_variable_use{u8"Type"}));
  }
}

TEST(test_parse_typescript_type, object_type_with_method) {
  {
    spy_visitor v = parse_and_visit_typescript_type(u8"{ method() }"_sv);
    EXPECT_THAT(v.visits, ElementsAre("visit_enter_function_scope",   // method
                                      "visit_exit_function_scope"));  // method
  }

  {
    spy_visitor v =
        parse_and_visit_typescript_type(u8"{ method(param: Type) }"_sv);
    EXPECT_THAT(v.visits, ElementsAre("visit_enter_function_scope",   // method
                                      "visit_variable_type_use",      // Type
                                      "visit_variable_declaration",   // param
                                      "visit_exit_function_scope"));  // method
  }

  {
    spy_visitor v =
        parse_and_visit_typescript_type(u8"{ method(): ReturnType }"_sv);
    EXPECT_THAT(v.visits, ElementsAre("visit_enter_function_scope",  // method
                                      "visit_variable_type_use",  // ReturnType
                                      "visit_exit_function_scope"));  // method
  }
}

TEST(test_parse_typescript_type, object_type_with_computed_property) {
  {
    spy_visitor v = parse_and_visit_typescript_type(u8"{ ['prop'] }"_sv);
    EXPECT_THAT(v.visits, IsEmpty());
  }

  {
    spy_visitor v = parse_and_visit_typescript_type(u8"{ ['prop']: Type }"_sv);
    EXPECT_THAT(v.visits, ElementsAre("visit_variable_type_use"));  // Type
  }

  {
    spy_visitor v = parse_and_visit_typescript_type(u8"{ ['method']() }"_sv);
    EXPECT_THAT(v.visits, ElementsAre("visit_enter_function_scope",   // method
                                      "visit_exit_function_scope"));  // method
  }

  {
    spy_visitor v = parse_and_visit_typescript_type(u8"{ [varName]: Type }"_sv);
    EXPECT_THAT(v.visits, ElementsAre("visit_variable_use",         // varName
                                      "visit_variable_type_use"));  // Type
    EXPECT_THAT(v.variable_uses,
                ElementsAre(spy_visitor::visited_variable_use{u8"varName"},
                            spy_visitor::visited_variable_use{u8"Type"}));
  }

  {
    spy_visitor v =
        parse_and_visit_typescript_type(u8"{ [ns.varName]: Type }"_sv);
    // TODO(strager): Should this be a namespace use instead of a runtime use?
    EXPECT_THAT(v.visits, ElementsAre("visit_variable_use",         // ns
                                      "visit_variable_type_use"));  // Type
    EXPECT_THAT(v.variable_uses,
                ElementsAre(spy_visitor::visited_variable_use{u8"ns"},
                            spy_visitor::visited_variable_use{u8"Type"}));
  }
}

TEST(test_parse_typescript_type, object_type_with_index_signature) {
  {
    spy_visitor v =
        parse_and_visit_typescript_type(u8"{ [key: KeyType]: PropType }"_sv);
    EXPECT_THAT(v.visits, ElementsAre("visit_enter_index_signature_scope",  //
                                      "visit_variable_type_use",     // KeyType
                                      "visit_variable_declaration",  // key
                                      "visit_variable_type_use",     // PropType
                                      "visit_exit_index_signature_scope"));
    EXPECT_THAT(
        v.variable_declarations,
        ElementsAre(spy_visitor::visited_variable_declaration{
            u8"key", variable_kind::_parameter, variable_init_kind::normal}));
    EXPECT_THAT(v.variable_uses,
                ElementsAre(spy_visitor::visited_variable_use{u8"KeyType"},
                            spy_visitor::visited_variable_use{u8"PropType"}));
  }
}

TEST(test_parse_typescript_type, object_type_with_mapped_types) {
  {
    spy_visitor v =
        parse_and_visit_typescript_type(u8"{ [Key in Keys]: PropType }"_sv);
    EXPECT_THAT(v.visits, ElementsAre("visit_enter_index_signature_scope",  //
                                      "visit_variable_type_use",     // Keys
                                      "visit_variable_declaration",  // Key
                                      "visit_variable_type_use",     // PropType
                                      "visit_exit_index_signature_scope"));
    EXPECT_THAT(v.variable_declarations,
                ElementsAre(spy_visitor::visited_variable_declaration{
                    u8"Key", variable_kind::_generic_parameter,
                    variable_init_kind::normal}));
    EXPECT_THAT(v.variable_uses,
                ElementsAre(spy_visitor::visited_variable_use{u8"Keys"},
                            spy_visitor::visited_variable_use{u8"PropType"}));
  }

  {
    spy_visitor v = parse_and_visit_typescript_type(
        u8"{ [Key in Keys as KeyType]: PropType }"_sv);
    EXPECT_THAT(v.visits, ElementsAre("visit_enter_index_signature_scope",  //
                                      "visit_variable_type_use",     // Keys
                                      "visit_variable_declaration",  // Key
                                      "visit_variable_type_use",     // KeyType
                                      "visit_variable_type_use",     // PropType
                                      "visit_exit_index_signature_scope"));
    EXPECT_THAT(v.variable_declarations,
                ElementsAre(spy_visitor::visited_variable_declaration{
                    u8"Key", variable_kind::_generic_parameter,
                    variable_init_kind::normal}));
    EXPECT_THAT(v.variable_uses,
                ElementsAre(spy_visitor::visited_variable_use{u8"Keys"},
                            spy_visitor::visited_variable_use{u8"KeyType"},
                            spy_visitor::visited_variable_use{u8"PropType"}));
  }
}

TEST(test_parse_typescript_type, object_type_with_modified_optional) {
  for (string8 modifier : {u8"-?", u8"+?", u8"?"}) {
    {
      padded_string code(u8"{ [key: KeyType]" + modifier + u8": PropType }");
      SCOPED_TRACE(code);
      spy_visitor v = parse_and_visit_typescript_type(code.string_view());
      EXPECT_THAT(v.visits, ElementsAre("visit_enter_index_signature_scope",  //
                                        "visit_variable_type_use",  // KeyType
                                        "visit_variable_declaration",  // key
                                        "visit_variable_type_use",  // PropType
                                        "visit_exit_index_signature_scope"));
    }

    {
      padded_string code(u8"{ [Key in Keys]" + modifier + u8": PropType }");
      SCOPED_TRACE(code);
      spy_visitor v = parse_and_visit_typescript_type(code.string_view());
      EXPECT_THAT(v.visits, ElementsAre("visit_enter_index_signature_scope",  //
                                        "visit_variable_type_use",     // Keys
                                        "visit_variable_declaration",  // Key
                                        "visit_variable_type_use",  // PropType
                                        "visit_exit_index_signature_scope"));
    }

    {
      padded_string code(u8"{ [Key in Keys as KeyType]" + modifier +
                         u8": PropType }");
      SCOPED_TRACE(code);
      spy_visitor v = parse_and_visit_typescript_type(code.string_view());
      EXPECT_THAT(v.visits, ElementsAre("visit_enter_index_signature_scope",  //
                                        "visit_variable_type_use",     // Keys
                                        "visit_variable_declaration",  // Key
                                        "visit_variable_type_use",  // KeyType
                                        "visit_variable_type_use",  // PropType
                                        "visit_exit_index_signature_scope"));
    }
  }
}

TEST(test_parse_typescript_type, object_type_with_modified_readonly) {
  for (string8 modifier : {u8"-readonly", u8"+readonly", u8"readonly"}) {
    {
      padded_string code(u8"{ " + modifier + u8" [key: KeyType]: PropType }");
      SCOPED_TRACE(code);
      spy_visitor v = parse_and_visit_typescript_type(code.string_view());
      EXPECT_THAT(v.visits, ElementsAre("visit_enter_index_signature_scope",  //
                                        "visit_variable_type_use",  // KeyType
                                        "visit_variable_declaration",  // key
                                        "visit_variable_type_use",  // PropType
                                        "visit_exit_index_signature_scope"));
    }

    {
      padded_string code(u8"{ " + modifier + u8" [Key in Keys]: PropType }");
      SCOPED_TRACE(code);
      spy_visitor v = parse_and_visit_typescript_type(code.string_view());
      EXPECT_THAT(v.visits, ElementsAre("visit_enter_index_signature_scope",  //
                                        "visit_variable_type_use",     // Keys
                                        "visit_variable_declaration",  // Key
                                        "visit_variable_type_use",  // PropType
                                        "visit_exit_index_signature_scope"));
    }

    {
      padded_string code(u8"{ " + modifier +
                         u8" [Key in Keys as KeyType]: PropType }");
      SCOPED_TRACE(code);
      spy_visitor v = parse_and_visit_typescript_type(code.string_view());
      EXPECT_THAT(v.visits, ElementsAre("visit_enter_index_signature_scope",  //
                                        "visit_variable_type_use",     // Keys
                                        "visit_variable_declaration",  // Key
                                        "visit_variable_type_use",  // KeyType
                                        "visit_variable_type_use",  // PropType
                                        "visit_exit_index_signature_scope"));
    }
  }
}

TEST(test_parse_typescript_type, object_type_with_call_signature) {
  {
    spy_visitor v = parse_and_visit_typescript_type(u8"{ () }"_sv);
    EXPECT_THAT(v.visits, ElementsAre("visit_enter_function_scope",  //
                                      "visit_exit_function_scope"));
  }

  {
    spy_visitor v = parse_and_visit_typescript_type(
        u8"{ (param: ParamType): ReturnType }"_sv);
    EXPECT_THAT(v.visits, ElementsAre("visit_enter_function_scope",  //
                                      "visit_variable_type_use",  // ParamType
                                      "visit_variable_declaration",  // param
                                      "visit_variable_type_use",  // ReturnType
                                      "visit_exit_function_scope"));
    EXPECT_THAT(
        v.variable_declarations,
        ElementsAre(spy_visitor::visited_variable_declaration{
            u8"param", variable_kind::_parameter, variable_init_kind::normal}));
    EXPECT_THAT(v.variable_uses,
                ElementsAre(spy_visitor::visited_variable_use{u8"ParamType"},
                            spy_visitor::visited_variable_use{u8"ReturnType"}));
  }
}

TEST(test_parse_typescript_type, object_type_with_keyword_named_properties) {
  for (string8 keyword : keywords) {
    {
      padded_string code(u8"{ " + keyword + u8" }");
      SCOPED_TRACE(code);
      spy_visitor v = parse_and_visit_typescript_type(code.string_view());
      EXPECT_THAT(v.visits, IsEmpty());
    }

    {
      padded_string code(u8"{ " + keyword + u8"() }");
      SCOPED_TRACE(code);
      spy_visitor v = parse_and_visit_typescript_type(code.string_view());
      EXPECT_THAT(v.visits, ElementsAre("visit_enter_function_scope",  //
                                        "visit_exit_function_scope"));
    }

    {
      padded_string code(u8"{ " + keyword + u8": Type }");
      SCOPED_TRACE(code);
      spy_visitor v = parse_and_visit_typescript_type(code.string_view());
      EXPECT_THAT(v.visits, ElementsAre("visit_variable_type_use"));  // Type
    }

    {
      padded_string code(u8"{ readonly " + keyword + u8": Type }");
      SCOPED_TRACE(code);
      spy_visitor v = parse_and_visit_typescript_type(code.string_view());
      EXPECT_THAT(v.visits, ElementsAre("visit_variable_type_use"));  // Type
    }

    {
      padded_string code(u8"{ " + keyword + u8"?: Type }");
      SCOPED_TRACE(code);
      spy_visitor v = parse_and_visit_typescript_type(code.string_view());
      EXPECT_THAT(v.visits, ElementsAre("visit_variable_type_use"));  // Type
    }
  }
}
}
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
