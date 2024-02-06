import winreg

def check_registry_key():
    try:
        key_path = r'SYSTEM\CurrentControlSet\services\LanmanWorkstation\Parameters'
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, key_path) as key:
            value_name = 'AllowInsecureGuestAuth'
            value, _ = winreg.QueryValueEx(key, value_name)
            return bool(value)
    except FileNotFoundError:
        print("The registry key does not exist.")
    except OSError:
        print(f"The value '{value_name}' does not exist in the registry key.")
    return False

check_registry_key()