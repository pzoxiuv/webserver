local server = {}

local codes = {[200] = "OK", [404] = "Not Found"};
local header = ""
local body = ""
local bodyType = STRING
local FILENAME = 0
local STRING = 1

server.port = ""
server.response = nil

-- Parses the request string into a table
server.parseReq = function(req)
	local reqTable = {}
    for line in req:gmatch("[^\r\n]*") do
        if reqTable["Method"] == nil then
            reqTable["Method"] = line:sub(0, (line:find(" ") or 0)-1)
            reqTable["URL"] = line:sub((line:find(" ") or 0)+1, (line:find(" ", (line:find(" ") or 0)+1) or 0)-1)
            reqTable["HTTPVer"] = line:sub((line:find(" [^ ]*$") or 0)+1)
        else
            for k, v in line:gmatch("([^:]*): ([^\r\n]*)") do reqTable[k] = v end
        end
    end
	return reqTable
end

-- Called from user created Lua file, builds an HTTP header string with the
-- user given table of options, plus the HTTP version, response code,
-- response code text, and the date.
function server.writeHeader(code, options)
	header = "HTTP/1.1 " .. code .. " " .. codes[code] .. "\r\n"
	for f, v in pairs(options) do
		header = header .. f .. ": " .. v .. "\r\n"
	end
	header = header .. "Date: " .. os.date("!%a, %d %b %Y %X GMT") .. "\r\n"
end

-- Only one of writeBodyStr or writeBodyFile may be called,
-- as body should either be a plain string or the filename of
-- the file to be sent.
function server.writeBodyStr(str)
	bodyType = STRING
	body = str
end

function server.writeBodyFile(filename)
	bodyType = FILENAME
	body = filename
end

-- Called from user created Lua file, takes in the port to listen on
-- and the function that is called to build a response.
function server.create(port_in, res)
	server.response = res
	server.port = server.port .. port_in
end

-- Takes in a request string, parses it into a table, passes the
-- table to the user defined function to build the response.
-- Returns the header, the body type, and then the body.
-- Body type is either 'STRING' (which will be sent as is, no further modifications),
-- or 'FILENAME' (which indicates the file to be opened and contents sent).
function server.handleReq(req)
	server.response(server.parseReq(req))
	return header .. "\r\n", body, bodyType
end

return server
