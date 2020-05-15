add_custom_target(npm-target ALL
  COMMAND cd ${PROJECT_SOURCE_DIR} && cd src/viewer/web/pages/fwview && npm install && npm run build
)
