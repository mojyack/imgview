project('imgview', 'cpp', version : '1.3.4', default_options : ['warning_level=3', 'werror=false', 'cpp_std=c++20'])
add_project_arguments('-Wfatal-errors', language: 'cpp')

subdir('src')

deps = [
  dependency('gawl', version : ['>=2.6', '<2.7']),
  dependency('gawl-fc'),
  dependency('gawl-keysym'),
  dependency('fontconfig'),
  dependency('Magick++'),
]

executable('imgview', [imgview_files],
            dependencies : deps,
            install : true)
