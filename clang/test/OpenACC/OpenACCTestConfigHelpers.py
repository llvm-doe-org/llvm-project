# Map of offload target triples to their abbreviations used before
# "-registered-target" in config.available_features.
offTgtToReg = {
  # directives/<TRIPLE>:       <ABBREV>-registered-target
  'x86_64-pc-linux-gnu':       'x86',
  'powerpc64le-ibm-linux-gnu': 'powerpc',
  'nvptx64-nvidia-cuda':       'nvptx',
  'amdgcn-amd-amdhsa':         'amdgpu'}

# Convert a target triple to omp_device_t spelling.  Returns "host" for
# tgt="host".
def tgtToOMP(tgt):
    tgt_short = tgt.split('-')[0]
    if tgt_short == "powerpc64le":
        return "ppc64le"
    return tgt_short

# Does this target triple support stdio (like printf)?  Returns true for
# tgt="host".
def tgtSupportsStdio(tgt):
    # FIXME: amdgcn should eventually support printf in a target region.
    return tgtToOMP(tgt) != "amdgcn"

# Return the Clang command-line option to specify the .bc directory for the
# target triple, or return empty string if none.
def tgtBcPathClangOpt(tgt, libomptarget_dir):
    if tgt == "nvptx64-nvidia-cuda":
        return "--libomptarget-nvptx-bc-path=" + libomptarget_dir
    if tgt == "amdgcn-amd-amdhsa":
        return "--libomptarget-amdgcn-bc-path=" + libomptarget_dir
    return ""

# Perform the algorithm described in
# clang/test/OpenACC/directives/Tests/lit.local.cfg for building FileCheck
# prefixes from a sequence of FileCheck prefix component sets.
#
# Paramters are:
# * prefixes = the starting set of FileCheck prefixes.  It must be a list of
#   strings.
# * component_sets = the sequence of FileCheck prefix component sets.  It must
#   be a list of lists of strings.
def addPrefixComponents(prefixes, component_sets):
    prefixes_new = prefixes
    for component_set in component_sets:
        prefixes_new += [prefix + '-' + component for prefix in prefixes_new
                         for component in component_set]
    return prefixes_new

# Same as addPrefixComponents except extract arguments from a python match
# object where:
# * The "prefixes" argument to addPrefixComponents is taken from the
#   comma-separated list in the match group "prefixes".
# * The "component_sets" argument to addPrefixComponents contains only one
#   component set, which is taken from the comma-separated list in the match
#   group "component_set".
def addPrefixComponentsFromMatch(match):
    prefixes = match.group('prefixes').split(',')
    component_set = match.group('component_set')
    component_set = component_set.split(',') if component_set else []
    return ','.join(addPrefixComponents(prefixes, [component_set]))
