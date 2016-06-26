import sys
try:
  import sconsutils
except ImportError:
  raise ImportError("Failed to import sconsutils (is buildtools on PYTHONPATH?)"), None, sys.exc_traceback

env = sconsutils.getEnv()
env.Append(LINKFLAGS = {'gcc': ["-Wl,--stack,134217728"]}[env['tool']])
env.InVariantDir(env['oDir'], ".", lambda env: env.LibAndApp('autoinf', 0, -1, (
  ('core', 0, 0),
  ('bitset', 0, 0),
  ('autofrotz', 0, 0)
)))
