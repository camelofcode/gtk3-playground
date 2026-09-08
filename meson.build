project(
  'gtk3-playground',
  'c',
  version: '1.0',
  default_options: [
    'c_std=c17',
    'warning_level=2',
  ]
)

gtk = dependency('gtk+-3.0', version: '>=3.24')

gnome = import('gnome')

resources = gnome.compile_resources(
  'gtk3-playground-resources',
  './resources/resources.gresource.xml',
  source_dir: '.'
)

sources = files(
  'src/main.c',
  'src/ui/windows/main_window.c',
  'src/ui/windows/text_binding_window.c',
  'src/ui/windows/task_list_window.c',
  'src/ui/widgets/task_row.c',
)

executable(
  'gtk3-playground',
  sources,
  resources,
  dependencies: gtk,
  export_dynamic: true,
)
