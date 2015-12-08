import os, imp
sconsutils = imp.load_source('sconsutils', os.path.join(os.pardir, os.pardir, "build", "sconsutils.py"))

env = sconsutils.getEnv()
env.Append(LINKFLAGS = {'gcc': ["-Wl,--stack,134217728"]}[env['tool']])
env.InVariantDir(env['oDir'], ".", lambda env: env.LibAndApp('autoinf', 0, -1, (
  ('core', 0, 0),
  ('bitset', 0, 0),
  ('autofrotz', 0, 0)
)))
