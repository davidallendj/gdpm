
# set up variables and glob files
env             = Environment(
    CPPPATH     = Glob('include/*.hpp'),
    CPPDEFINES  = [],
    LIBS        = []
)
base_name       = "gdpm"
include_files   = Glob('include/*.hpp')
source_files    = Glob('src/*.cpp')
test_files      = Glob('tests/*.cpp')
compile_flags   = ""

# build the main executable and tests
env.Program(f'{base_name}', source_files)
env.Program(f'{base_name}.tests', test_files)

# build the static library
StaticLibrary(f'{base_name}-static')

# build the shared libraries
SharedLibrary(f'{base_name}-shared')
SharedLibrary(f'{base_name}-static')
SharedLibrary(f'{base_name}-http')
SharedLibrary(f'{base_name}-restapi')
