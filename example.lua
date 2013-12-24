server = require("server")

server.create(8888, function(req)
		if req["URL"] == "/index.html" then
			server.writeBodyFile("index.html")
			server.writeHeader(200, {["Connection"] = "close", ["Content-Type"] = "text/html"})
		else
			server.writeBodyStr("Page not found!")
			server.writeHeader(404, {["Content-Length"] = 15, ["Connection"] = "close", ["Content-Type"] = "text/html"})
		end
	end
)
