import struct
import ctypes

def inject_icon(exe_path, ico_path):
    with open(ico_path, 'rb') as f:
        ico_data = f.read()
    reserved, ico_type, count = struct.unpack('<HHH', ico_data[:6])
    if reserved != 0 or ico_type != 1:
        return False
    kernel32 = ctypes.windll.kernel32
    kernel32.BeginUpdateResourceW.argtypes = [ctypes.c_wchar_p, ctypes.c_bool]
    kernel32.BeginUpdateResourceW.restype = ctypes.c_void_p
    kernel32.UpdateResourceW.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_ushort, ctypes.c_void_p, ctypes.c_ulong]
    kernel32.UpdateResourceW.restype = ctypes.c_bool
    kernel32.EndUpdateResourceW.argtypes = [ctypes.c_void_p, ctypes.c_bool]
    kernel32.EndUpdateResourceW.restype = ctypes.c_bool
    
    h_update = kernel32.BeginUpdateResourceW(exe_path, False)
    if not h_update:
        return False
    RT_ICON = ctypes.c_void_p(3)
    RT_GROUP_ICON = ctypes.c_void_p(14)
    grp_dir_data = bytearray(ico_data[:6])
    offset = 6
    for i in range(count):
        entry = ico_data[offset:offset+16]
        width, height, colors, reserved_byte, planes, bpp, size, image_offset = struct.unpack('<BBBBHHII', entry)
        img_data = ico_data[image_offset:image_offset+size]
        icon_id = i + 1
        res_name = ctypes.c_void_p(icon_id)
        char_array = (ctypes.c_char * len(img_data)).from_buffer_copy(img_data)
        kernel32.UpdateResourceW(h_update, RT_ICON, res_name, 0, ctypes.byref(char_array), len(img_data))
        grp_entry = struct.pack('<BBBBHHIH', width, height, colors, reserved_byte, planes, bpp, size, icon_id)
        grp_dir_data.extend(grp_entry)
        offset += 16
    grp_name = ctypes.c_void_p(101)
    grp_char_array = (ctypes.c_char * len(grp_dir_data)).from_buffer_copy(grp_dir_data)
    kernel32.UpdateResourceW(h_update, RT_GROUP_ICON, grp_name, 0, ctypes.byref(grp_char_array), len(grp_dir_data))
    kernel32.EndUpdateResourceW(h_update, False)
    print("Injected successfully!")
    return True

inject_icon('StreamlineDict.exe', 'icon\\translate.ico')
