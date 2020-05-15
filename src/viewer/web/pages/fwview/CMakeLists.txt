add_custom_target(npm-install
  cd ${PROJECT_SOURCE_DIR}/src/viewer/web/pages/fwview && npm install
  COMMENT "npm install"
)

add_custom_target(npm-build-target ALL
  cd ${PROJECT_SOURCE_DIR}/src/viewer/web/pages/fwview && npm run build
  COMMENT "npm run build"
  DEPENDS src/index.js src/index.css npm-install
)

# add_custom_command(OUTPUT bin/pages/
#   COMMAND cd ${PROJECT_SOURCE_DIR}/src/viewer/web/pages/fwview && npm run build
#   COMMENT "npm run build"
#   DEPENDS src/index.js src/index.css npm-install
# )
