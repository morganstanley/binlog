if (DEFINED BINLOG_SOURCE_BROWSER_URL)
  set(SOURCE_BROWSER_SWITCH --source-browser-base-url)
endif()

# Creates a target which converts doc/`doc_name`.md to HTML
function(markdown_to_html doc_name)
  add_custom_command(
    OUTPUT "${doc_name}.html"
    COMMENT "Generate ${doc_name}.html"
    COMMAND "python" "gendoc.py"
                     "--catchfile-dir" "${PROJECT_SOURCE_DIR}"
                     "${SOURCE_BROWSER_SWITCH}" "${BINLOG_SOURCE_BROWSER_URL}"
                     "<" "${doc_name}.md"
                     ">" "${CMAKE_CURRENT_BINARY_DIR}/${doc_name}.html"
    DEPENDS "doc/${doc_name}.md" "doc/gendoc.py" "doc/body.html"
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/doc/
  )

  add_custom_target(${doc_name} DEPENDS "${doc_name}.html")
endfunction()

# Takes a group name and any number of document names,
# calls markdown_to_html for each document,
# and adds a new target `group_name`, which
# depends on all of those.
function(markdown_to_html_group group_name)
  list(REMOVE_AT ARGV 0)
  foreach(doc_name ${ARGV})
    markdown_to_html(${doc_name})
  endforeach()

  add_custom_target(${group_name} DEPENDS ${ARGV})
endfunction()
