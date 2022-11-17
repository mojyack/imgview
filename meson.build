project('imgview', 'cpp', version : '1.3.1', default_options : ['warning_level=3', 'werror=false', 'cpp_std=c++20'])

subdir('src')

deps = [
  dependency('gawl', version : ['>=2.4', '<2.5']),
  dependency('gawl-keysym'),
  dependency('fontconfig'),
  dependency('Magick++'),
]

executable('imgview', [imgview_files],
            dependencies : deps,
            install : true)
