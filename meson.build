project('imgview', 'cpp', default_options : ['warning_level=3', 'werror=false', 'cpp_std=c++2a'])

subdir('src')

executable('imgview', [imgview_files],
            dependencies : [dependency('gawl')],
            install : true)
