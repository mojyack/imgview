project('imgview', 'cpp', version : '1.1.0', default_options : ['warning_level=3', 'werror=false', 'cpp_std=c++20'])

subdir('src')

executable('imgview', [imgview_files],
            dependencies : [dependency('gawl', version : ['>=1.6', '<1.7'])],
            install : true)
