project('imgview', 'cpp', default_options : ['warning_level=3', 'werror=false', 'cpp_std=c++2a'])

subdir('src')

executable('imgview', [imgview_files],
            include_directories: [imgview_inc],
            dependencies : [imgview_dep])
