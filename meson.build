project('imgview', 'c', 'cpp', version : '2.0.2', default_options : ['warning_level=3', 'werror=false', 'cpp_std=c++20'])
add_project_arguments('-Wfatal-errors', language: 'cpp')

subdir('src')

executable('imgview', imgview_files,
  dependencies : imgview_deps,
  install : true,
)
