import os
import subprocess
import time
import win32gui
import win32con
import win32com.client
import win32api
from pathlib import Path

# Constants for SelectItem
SVSI_ENSUREVISIBLE = 0x8
SVSI_FOCUSED = 0x10
SVSI_SELECTIONMARK = 0x40

def scroll_to_center(hwnd, steps=5):
    """Helper function to scroll the view by a certain number of steps"""
    for _ in range(steps):
        win32api.SendMessage(hwnd, win32con.WM_VSCROLL, win32con.SB_LINEDOWN, 0)
        time.sleep(0.05)

def open_explorer_centered(file_path):
    """
    Opens Windows Explorer, selects a specific file, and ensures it's centered in view.
    
    Args:
        file_path (str): The full path to the file you want to focus on
    """
    if not os.path.exists(file_path):
        raise FileNotFoundError(f"File not found: {file_path}")
    
    # Normalize path
    file_path = os.path.normpath(file_path)
    
    # First, open Explorer and select the file
    subprocess.run(['explorer', '/select,', file_path])
    
    # Give Explorer time to open
    time.sleep(0.5)
    
    # Use Shell.Application to get Explorer window
    shell = win32com.client.Dispatch("Shell.Application")
    
    # Get the parent folder path
    parent_folder = str(Path(file_path).parent)
    filename = os.path.basename(file_path)
    
    # Get shell folder object for the parent directory
    shell_folder = shell.NameSpace(parent_folder)
    if not shell_folder:
        return
    
    # Find the Explorer window
    for window in shell.Windows():
        try:
            if str(Path(window.Document.Folder.Self.Path)) == parent_folder:
                # Get the folder view object
                folder_items = shell_folder.Items()
                
                # Find and select our file
                for item in folder_items:
                    if item.Name == filename:
                        # Focus the window
                        hwnd = win32gui.FindWindow("CabinetWClass", None)
                        if hwnd:
                            # Bring window to front
                            win32gui.SetForegroundWindow(hwnd)
                            
                            # Force a refresh
                            win32gui.SendMessage(hwnd, win32con.WM_SYSCOMMAND, win32con.SC_RESTORE, 0)
                            
                            # Get list view control handle
                            list_view = win32gui.FindWindowEx(hwnd, 0, "ShellView", None)
                            if list_view:
                                # First ensure item is visible
                                window.Document.SelectItem(
                                    item, 
                                    SVSI_ENSUREVISIBLE | SVSI_FOCUSED | SVSI_SELECTIONMARK
                                )
                                time.sleep(0.2)  # Give it time to scroll
                                
                                # Now try to center it
                                # First scroll to top
                                win32api.SendMessage(list_view, win32con.WM_VSCROLL, win32con.SB_TOP, 0)
                                time.sleep(0.2)
                                
                                # Make item visible again
                                window.Document.SelectItem(
                                    item, 
                                    SVSI_ENSUREVISIBLE | SVSI_FOCUSED | SVSI_SELECTIONMARK
                                )
                                time.sleep(0.2)
                                
                                # Now scroll down a bit to center it
                                scroll_to_center(list_view, steps=8)  # Adjust steps as needed
                        
                        break
                break
        except Exception as e:
            print(f"Error while processing window: {e}")
            continue

open_explorer_centered("D:\\Donwloads\\rapidsave.com_-dxfxi0wi43td1.mp4")
