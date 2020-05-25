add_custom_target(npm-install
  cd ${PROJECT_SOURCE_DIR}/src/viewer/web/pages/fwview && npm install
  COMMENT "npm install"
  DEPENDS package.json
)

add_custom_target(npm-build-target
  cd ${PROJECT_SOURCE_DIR}/src/viewer/web/pages/fwview && npm run build
  COMMENT "npm run build"
  DEPENDS src/index.js src/index.css
  )

add_dependencies(npm-build-target npm-install)

# add_custom_command(OUTPUT bin/pages/
#   COMMAND cd ${PROJECT_SOURCE_DIR}/src/viewer/web/pages/fwview && npm run build
#   COMMENT "npm run build"
#   DEPENDS src/index.js src/index.css npm-install
# )
