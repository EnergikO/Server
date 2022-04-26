
#define SFML_STATIC
#include <SFML/Network.hpp>
#include <simdjson.h>

#include <iostream>
#include <list>
#include <thread>
#include <unordered_set>

#pragma comment(lib, "sfml-system-s-d.lib")
#pragma comment(lib, "sfml-network-s-d.lib")

#pragma comment(lib, "simdjson.lib")

#pragma comment(lib, "ws2_32.lib")



static const char* IP = "25.74.101.133";
static constexpr uint16_t PORT = 9090;



bool receive_string(sf::TcpSocket& sock, std::string& result)
{
	char arr[32];
	size_t received = 0;
	if (sock.receive(arr, 31, received) != sf::Socket::Status::Done || received == 0)
		return false;
	arr[received] = 0;

	size_t length = 0;
	if (!(std::istringstream(arr) >> length))
		return false;

	if (sock.send("ok", 2) != sf::Socket::Status::Done)
		return false;

	result.clear();
	result.resize(length);
	if (sock.receive(result.data(), length, received) != sf::Socket::Status::Done || received != length)
		return false;

	return true;
}
std::string receive_string(sf::TcpSocket& sock)
{
	std::string result;
	if (!receive_string(sock, result))
		result.clear();
	return result;
}

bool send_string(sf::TcpSocket& sock, const std::string& result)
{
	char arr[32];
	size_t _;
	snprintf(arr, sizeof(arr), "%zu", result.size());

	if (sock.send(arr, strlen(arr)) != sf::Socket::Status::Done)
		return false;

	if (sock.receive(arr, 2, _) != sf::Socket::Status::Done || strncmp(arr, "ok", 2) != 0)
		return false;

	if (sock.send(result.data(), result.size()) != sf::Socket::Status::Done)
		return false;

	return true;
}



void json_stream_add_string(std::ostream& os, const std::string& s)
{
	os << '"' << s << '"';
}
void json_stream_add_string_pair(std::ostream& os, const std::string& name, const std::string& value, bool comma = true)
{
	json_stream_add_string(os, name);
	os << ':';
	json_stream_add_string(os, value);
	if (comma)
		os << ',';
}



std::string request(sf::TcpSocket& sock, const std::string& name, const std::string& data = "")
{
	std::ostringstream ss;
	ss << '{';
	json_stream_add_string_pair(ss, "request_name", name, !data.empty());

	if (!data.empty())
	{
		json_stream_add_string(ss, "request_args");
		ss << ":{";
		ss << data;
		ss << "}";
	}

	ss << "}";

	if (!send_string(sock, ss.str()))
		return "";

	return receive_string(sock);
}

bool request_db_connection(sf::TcpSocket& sock, const std::string& name)
{
	std::ostringstream request_data;
	json_stream_add_string_pair(request_data, "db_name", name, false);

	std::string response = request(sock, "db_connect", request_data.str());

	return response.length() == 2;
}
bool request_db_disconnect(sf::TcpSocket& sock, const std::string& name)
{
	std::ostringstream request_data;
	json_stream_add_string_pair(request_data, "db_name", name, false);

	std::string response = request(sock, "db_disconnect", request_data.str());

	return response.length() == 2;
}

#define assert_return_false(expr) if (!(expr)) return false; else [](){}

bool request_db_query(sf::TcpSocket& sock, std::string& result, const std::string& dbname, const std::string& table, const std::string& filter_condition, const std::string& projection)
{
	std::ostringstream request_data;
	json_stream_add_string_pair(request_data, "db_name", dbname);
	json_stream_add_string_pair(request_data, "table", table);
	json_stream_add_string_pair(request_data, "condition", filter_condition);
	json_stream_add_string_pair(request_data, "fields", projection, false);

	result = request(sock, "db_get_fields_by", request_data.str());
	if (result.length() >= 2)
	{
		result.pop_back();
		result.pop_back();
		result.erase(0, result.find('[', 8) + 1);
	}
	return true;
}

bool request_db_add(sf::TcpSocket& sock, std::string& result, const std::string& dbname, const std::string& table, const std::string& fields, const std::string& values)
{
	std::ostringstream request_data;
	json_stream_add_string_pair(request_data, "db_name", dbname);
	json_stream_add_string_pair(request_data, "table", table);
	json_stream_add_string_pair(request_data, "values", values);
	json_stream_add_string_pair(request_data, "fields", fields, false);

	result = request(sock, "db_add", request_data.str());
	std::cout << result << std::endl;
	return result.length() == 2;
}

void request_terminate(sf::TcpSocket& sock)
{
	request(sock, "terminate");
}



int server_core(sf::TcpSocket& server)
{
	const std::string dbname = "pictures.db";
	const std::string table = "_temp_pictures";

	std::unordered_set<std::string> active_db_connections;
	std::string result;

	if (!request_db_connection(server, dbname))
		return 3;

	request(server, "db_execute", "\"db_name\":\"" + dbname + "\",\"command\": \"SELECT * FROM _temp_pictures;\"");

	request_db_add(server, result, dbname, table, "id, filename", "\\\"abo\\\", \\\"ba\\\"");
	request_db_query(server, result, dbname, table, "id == \\\"abo\\\"", "filename, id");

	request_db_disconnect(server, dbname);

	return 0;
}

int main()
{
	sf::TcpSocket server;
	while (true)
	{
		auto status = server.connect(IP, PORT, sf::seconds(2));
		if (status == sf::Socket::Done)
			break;
	}

	std::cout << "Connected to " << server.getRemoteAddress() << ':' << server.getRemotePort() << std::endl;
	int code = server_core(server);
	request_terminate(server);
	return code;
}
