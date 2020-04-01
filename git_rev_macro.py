import subprocess

revision = subprocess.check_output(["git", "rev-parse", "HEAD"]).strip()
# print("-D PIO_SRC_REV=%s" % revision)
print( "-D GIT_SRC_REV=" + revision.decode('UTF-8') )