from building import *

cwd = GetCurrentDir()
path = [cwd + '/inc']
src  = Glob('src/*.c')

# W60X
if GetDepend(['MY_AT_DEVICE_USING_W60X']):
    path += [cwd + '/class/w60x']
    src += Glob('class/w60x/at_device_w60x.c')
    if GetDepend(['AT_USING_SOCKET']):
        src += Glob('class/w60x/at_socket_w60x.c')
    if GetDepend(['MY_AT_DEVICE_W60X_SAMPLE']):
        src += Glob('samples/at_sample_w60x.c')

group = DefineGroup('at_device', src, depend = ['PKG_USING_MY_AT_DEVICE'], CPPPATH = path)

Return('group')
