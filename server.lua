local server = {}

local codes = {[200] = "OK", ["404"] = "Not found"};
local header = ""
server.buildHeader = nil

function server.writeHeader(code, options)
	header = "HTTP/1.1 " .. code .. " " .. codes[code] .. "\r\n"
	for f, v in pairs(options) do
		header = header .. f .. ": " .. v .. "\r\n"
	end
end

function server.parseReq(req)
	server.buildHeader(req)
	header = header .. "Date: " .. os.date("!%a, %d %b %Y %X GMT") .. "\r\n"
	header = header .. "\r\n"
	return header
end

return server