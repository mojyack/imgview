project('imgview', 'cpp', version : '1.2.1', default_options : ['warning_level=3', 'werror=false', 'cpp_std=c++20'])

subdir('src')

executable('imgview', [imgview_files],
            dependencies : [dependency('gawl', version : ['>=2.2', '<2.3'])],
            install : true)
