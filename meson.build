project('imgview', 'cpp', version : '1.0.0', default_options : ['warning_level=3', 'werror=false', 'cpp_std=c++2a'])

subdir('src')

executable('imgview', [imgview_files],
            dependencies : [dependency('gawl', version : ['>=1.0', '<1.1'])],
            install : true)
