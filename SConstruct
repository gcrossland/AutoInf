import sys
try:
  import sconsutils
except ImportError:
  raise ImportError("Failed to import sconsutils (is buildtools on PYTHONPATH?)"), None, sys.exc_traceback

env = sconsutils.getEnv()
env.Append(
  CXXFLAGS = {'gcc-posix': []}.get(env['tool'] + "-" + env['os'], []), # TODO -fsplit-stack
  LINKFLAGS = {'gcc-win32': ["-Wl,--stack,134217728"]}.get(env['tool'] + "-" + env['os'], [])
)
env.InVariantDir(env['oDir'], ".", lambda env: env.LibAndApp('autoinf', 0, -1, (
  ('core', 0, 0),
  ('bitset', 0, 0),
  ('autofrotz', 0, 0),
  ('iterators', 0, 0)
)))
