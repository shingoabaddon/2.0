# Flashes this console window's taskbar button a few times - called by
# clean_git_locks.bat once per repeat cycle, purely as a "still running"
# reminder. Kept as its own .ps1 (rather than inline in the .bat, like the
# rest of that script's PowerShell calls) because FlashWindowEx's P/Invoke
# declaration needs literal double-quoted C# strings (DllImport("...")),
# which don't survive being embedded in a cmd.exe double-quoted -Command
# argument without fragile escaping - a here-string in a real .ps1 file
# sidesteps that entirely.

Add-Type -Namespace Fox -Name FlashUtil -MemberDefinition @'
[DllImport("kernel32.dll")]
public static extern IntPtr GetConsoleWindow();

[StructLayout(LayoutKind.Sequential)]
public struct FLASHWINFO {
    public uint cbSize;
    public IntPtr hwnd;
    public uint dwFlags;
    public uint uCount;
    public uint dwTimeout;
}

[DllImport("user32.dll")]
public static extern bool FlashWindowEx(ref FLASHWINFO pwfi);
'@ -UsingNamespace System.Runtime.InteropServices

$hwnd = [Fox.FlashUtil]::GetConsoleWindow()
$fi = New-Object Fox.FlashUtil+FLASHWINFO
$fi.cbSize = [System.Runtime.InteropServices.Marshal]::SizeOf($fi)
$fi.hwnd = $hwnd
$fi.dwFlags = 3      # FLASHW_ALL (flash both the caption and the taskbar button)
$fi.uCount = 5        # flash 5 times, then stop on its own
$fi.dwTimeout = 0     # 0 = use the default cursor blink rate
[Fox.FlashUtil]::FlashWindowEx([ref]$fi) | Out-Null
