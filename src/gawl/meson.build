wl_deps = [
    dependency('wayland-client++'), 
    dependency('wayland-client-extra++'), 
    dependency('wayland-cursor++'), 
    dependency('wayland-egl++')]
egl_deps = [
    dependency('glx'),
    dependency('opengl'),
    dependency('egl')]
freetype_inc = include_directories('/usr/include/freetype2')
freetype_dep = dependency('freetype2')
thread_dep = dependency('threads')
magick_inc = include_directories('/usr/include/ImageMagick-7')
magick_dep = dependency('Magick++')
devil_dep = dependency('IL')

gawl_inc = [freetype_inc, magick_inc]
gawl_dep = [wl_deps, egl_deps, freetype_dep, thread_dep, magick_dep, devil_dep]
gawl_files = files(
    'gawl-application.cpp',
    'gawl-window.cpp',
    'graphic-base.cpp',
    'graphic.cpp',
    'misc.cpp',
    'textrender.cpp',
    'wayland-application.cpp',
    'wayland-window.cpp',
)
