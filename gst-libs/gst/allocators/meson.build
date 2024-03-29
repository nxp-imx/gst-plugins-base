gst_allocators_headers = files([
  'allocators.h',
  'allocators-prelude.h',
  'gstfdmemory.h',
  'gstphysmemory.h',
  'gstdmabuf.h',
  'gstdmabufmeta.h',
  'gstphymemmeta.h',
  'gstallocatorphymem.h',
])

if cc.has_header('linux/ion.h')
  gst_allocators_headers += [
    'gstionmemory.h',
  ]
endif

if cc.has_header('linux/dma-heap.h')
  gst_allocators_headers += [
    'gstdmabufheaps.h',
  ]
endif

install_headers(gst_allocators_headers, subdir : 'gstreamer-1.0/gst/allocators/')

gst_allocators_sources = files([ 'gstdmabuf.c', 'gstfdmemory.c', 'gstphysmemory.c', 'gstdmabufmeta.c', 'gstphymemmeta.c', 'gstallocatorphymem.c'])

if cc.has_header('linux/ion.h')
  gst_allocators_sources += [
    'gstionmemory.c',
  ]
endif

if cc.has_header('linux/dma-heap.h')
  gst_allocators_sources += [
    'gstdmabufheaps.c',
  ]
endif

gstallocators = library('gstallocators-@0@'.format(api_version),
  gst_allocators_sources,
  c_args : gst_plugins_base_args + ['-DBUILDING_GST_ALLOCATORS', '-DG_LOG_DOMAIN="GStreamer-Allocators"'],
  include_directories: [configinc, libsinc],
  version : libversion,
  soversion : soversion,
  darwin_versions : osxversion,
  install : true,
  dependencies : [gst_dep, video_dep],
)

pkg_name = 'gstreamer-allocators-1.0'
pkgconfig.generate(gstallocators,
  libraries : [gst_dep],
  variables : pkgconfig_variables,
  subdirs : pkgconfig_subdirs,
  name : pkg_name,
  description : 'Allocators implementation',
)

library_def = {'lib': gstallocators}
allocators_gen_sources  = []
if build_gir
  gst_gir_extra_args = gir_init_section + [ '--c-include=gst/allocators/allocators.h' ]
  gir = {
    'sources' : gst_allocators_sources + gst_allocators_headers,
    'namespace' : 'GstAllocators',
    'nsversion' : api_version,
    'identifier_prefix' : 'Gst',
    'symbol_prefix' : 'gst',
    'export_packages' : pkg_name,
    'includes' : ['Gst-1.0'],
    'install' : true,
    'extra_args' : gst_gir_extra_args,
    'dependencies' : [gst_dep]
  }
  library_def += {'gir': [gir]}
  if not static_build
    allocators_gir = gnome.generate_gir(gstallocators, kwargs: gir)
    allocators_gen_sources += allocators_gir
  endif
endif
gst_libraries += [[pkg_name, library_def]]

allocators_dep = declare_dependency(link_with: gstallocators,
  include_directories : [libsinc],
  dependencies : [gst_dep],
  sources : allocators_gen_sources)

meson.override_dependency('gstreamer-allocators-1.0', allocators_dep)
