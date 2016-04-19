local so = require "csocket"

local sk = so.socket(so.AF_INET, so.SOCK_STREAM, 0)
sk:setdebug(true)
print(sk:bind("104.224.137.96", "12222"))
print(sk:connect("104.224.137.96", "22222"))
io.read()
sk:send("hello")
io.read()
