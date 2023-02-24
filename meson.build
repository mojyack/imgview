project('imgview', 'cpp', version : '1.3.3', default_options : ['warning_level=3', 'werror=false', 'cpp_std=c++20'])

subdir('src')

deps = [
  dependency('gawl', version : ['>=2.5', '<2.6']),
  dependency('gawl-keysym'),
  dependency('fontconfig'),
  dependency('Magick++'),
]

executable('imgview', [imgview_files],
            dependencies : deps,
            install : true)
