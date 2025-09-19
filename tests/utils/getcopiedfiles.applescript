use framework "AppKit"

property this : a reference to current application
property NSPasteboard : a reference to NSPasteboard of this
property NSURL : a reference to NSURL of this

property text item delimiters : linefeed

set pb to NSPasteboard's generalPasteboard()
set fs to (pb's readObjectsForClasses:[NSURL] options:[]) as list

repeat with f in fs
    set f's contents to POSIX path of f
end repeat

fs as text