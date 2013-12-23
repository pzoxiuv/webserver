local server = {}

local codes = {[200] = "OK", ["404"] = "Not found"};
local header = ""
local body = ""

server.port = ""
server.response = nil

function server.writeHeader(code, options)
	header = "HTTP/1.1 " .. code .. " " .. codes[code] .. "\r\n"
	for f, v in pairs(options) do
		header = header .. f .. ": " .. v .. "\r\n"
	end
end

function server.writeBody(str)
	body = str
end

function server.create(port_in, res)
	server.response = res
	server.port = server.port .. port_in
end

function server.parseReq(req)
	server.response(req)
	header = header .. "Date: " .. os.date("!%a, %d %b %Y %X GMT") .. "\r\n"
	return header .. "\r\n" .. body
end

return server
