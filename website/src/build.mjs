// Copyright (C) 2020  Matthew "strager" Glazar
// See end of file for extended copyright information.

import assert from "assert";
import fs from "fs";
import path from "path";
import url from "url";
import { Router, isHiddenPath } from "./router.mjs";

export async function makeBuildInstructionsAsync({
  wwwRootPath,
  esbuildBundles = {},
  htmlRedirects = {},
}) {
  let router = new Router({
    wwwRootPath: wwwRootPath,
    esbuildBundles: esbuildBundles,
    htmlRedirects: htmlRedirects,
  });
  let instructions = [];
  await makeBuildInstructionsForESBuildBundlesAsync(router, instructions);
  await makeBuildInstructionsForHTMLRedirectsAsync(router, instructions);
  await makeBuildInstructionsImplAsync(router, instructions, "");
  return instructions;
}

async function makeBuildInstructionsForESBuildBundlesAsync(
  router,
  instructions
) {
  for (let [uri, esbuildConfig] of Object.entries(router.esbuildBundles)) {
    assert.deepStrictEqual(await router.classifyFileRouteAsync(uri), {
      type: "esbuild",
      esbuildConfig: esbuildConfig,
    });
    instructions.push({
      type: "esbuild",
      bundlePath: relativeURIToRelativePath(uri),
      esbuildConfig: esbuildConfig,
    });
  }
}

async function makeBuildInstructionsForHTMLRedirectsAsync(
  router,
  instructions
) {
  for (let [redirectFrom, redirectTo] of Object.entries(router.htmlRedirects)) {
    assert.deepStrictEqual(await router.classifyFileRouteAsync(redirectFrom), {
      type: "redirect",
      redirectTargetURL: redirectTo,
    });
    // TODO(strager): What if redirectFrom is something like "/foobar/"?
    instructions.push({
      type: "html-redirect",
      htmlPath: relativeURIToRelativePath(redirectFrom),
      redirectTargetURL: redirectTo,
    });
  }
}

async function makeBuildInstructionsImplAsync(router, instructions, basePath) {
  let files = await fs.promises.readdir(
    path.join(router.wwwRootPath, basePath),
    {
      encoding: "utf-8",
      withFileTypes: true,
    }
  );
  for (let file of files) {
    if (file.isDirectory()) {
      let directoryBasePath = path.join(basePath, file.name);
      if (!isHiddenPath(directoryBasePath)) {
        await makeBuildInstructionsImplAsync(
          router,
          instructions,
          directoryBasePath
        );
      }
    }
  }

  let classifiedDirectory = await router.classifyDirectoryAsync(basePath);
  await makeInstructionsForRouteAsync(classifiedDirectory, basePath);

  for (let file of files) {
    let relativePath = path.join(basePath, file.name);
    let classification = await router.classifyFileAsync(relativePath);
    await makeInstructionsForRouteAsync(classification, relativePath);
  }

  async function makeInstructionsForRouteAsync(route, relativePath) {
    switch (route.type) {
      case "ambiguous":
        instructions.push({
          type: "warning",
          message: `/${basePath} has both index.ejs.html and index.html; using neither`,
        });
        break;

      case "build-ejs":
        instructions.push({
          type: "build-ejs",
          sourcePath: route.path,
          destinationPath: path.join(basePath, "index.html"),
          ejsVariables: {
            currentURI: basePath === "" ? "/" : `/${basePath}/`,
          },
        });
        break;

      case "copy":
        instructions.push({
          type: "copy",
          path: route.path,
        });
        break;

      case "does-not-exist":
        break;

      case "missing":
        if (route.why === "broken-symlink") {
          instructions.push({
            type: "warning",
            message: `/${relativePath} is a broken symlink; ignoring`,
          });
        }
        break;

      case "index-script":
        let { routes } = await import(
          url.pathToFileURL(path.join(router.wwwRootPath, relativePath))
        );
        for (let routeURI in routes) {
          if (!Object.prototype.hasOwnProperty.call(routes, routeURI)) {
            continue;
          }
          let newRoute = routes[routeURI];
          if (newRoute.type !== "build-ejs") {
            throw new Error(`Unsupported route type: ${newRoute.type}`);
          }
          instructions.push({
            type: "build-ejs",
            sourcePath: path.join(basePath, routes[routeURI].path),
            destinationPath: path.join(
              relativeURIToRelativePath(routeURI),
              "index.html"
            ),
            ejsVariables: {
              currentURI: routeURI,
            },
          });
        }
        break;

      case "forbidden": // HACK(strager): .htaccess
      case "static":
        instructions.push({ type: "copy", path: relativePath });
        break;

      default:
        throw new Error(`Unexpected route type: ${route.type}`);
    }
  }
}

function relativeURIToRelativePath(uri) {
  return uri.replace(/^\//, "").replace(/\//g, path.sep);
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
