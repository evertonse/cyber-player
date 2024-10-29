import ctypes
import time

kernel32 = ctypes.windll.kernel32
user32   = ctypes.windll.user32

#standart clipboard formats used with win32
CF_TEXT   = 1
CF_BITMAP = 2

#lambda checks current data format with passed one
is_available_type = lambda type_val: user32.IsClipboardFormatAvailable(type_val)

def get_data(_type):
    #returns raw bytes from memorys
    data = user32.GetClipboardData(_type)
    data_locked = kernel32.GlobalLock(data) # get the pointer
    text = ctypes.c_char_p(data_locked)     # convert to python string
    kernel32.GlobalUnlock(data_locked)
    return text.value


#print text or save image
user32.OpenClipboard(0)
if is_available_type(CF_TEXT):
    data    = get_data(CF_TEXT)
    decoded = bytes.decode(data,encoding='utf-8')
    print(decoded)
elif is_available_type(CF_BITMAP):
    img = ImageGrab.grabclipboard()
    img.save(time.ctime().replace(':','..')+".png","PNG")
else:
    print('no text/image in clipboard')

user32.CloseClipboard()
