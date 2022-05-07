
#define SFML_STATIC
#include <SFML/Network.hpp>
#include <simdjson.h>

#include <iostream>
#include <fstream>
#include <list>
#include <thread>
#include <unordered_set>
#include <unordered_map>
#include <syncstream>
#include <chrono>
#include <filesystem>
#include <random>

#include <stdio.h>



#pragma comment(lib, "sfml-system-s-d.lib")
#pragma comment(lib, "sfml-network-s-d.lib")

#pragma comment(lib, "simdjson.lib")

#pragma comment(lib, "ws2_32.lib")


#pragma warning(disable : 4996)



static const char* DB_IP = "25.74.101.133";
static constexpr uint16_t DB_PORT = 9091;

static constexpr uint16_t SERVER_PORT = 9090;

static constexpr size_t message_length_digits = 8;


std::mt19937_64 mte(std::random_device{}());
//std::mt19937_64 mte();



bool receive_stream(sf::TcpSocket& sock, void* buffer, size_t size)
{
	uint8_t* p = (uint8_t*)buffer;
	while (size)
	{
		size_t received = 0;
		if (sock.receive(p, size, received) != sf::Socket::Status::Done)
			return false;

		size -= received;
		p += received;
	}
	return true;
}
bool send_stream(sf::TcpSocket& sock, const void* buffer, size_t size)
{
	const uint8_t* p = (uint8_t*)buffer;
	while (size)
	{
		size_t sent = 0;
		if (sock.send(p, size, sent) != sf::Socket::Status::Done)
			return false;

		size -= sent;
		p += sent;
	}
	return true;
}

FILE* _log = fopen("log.txt", "wb");

bool receive_string1(sf::TcpSocket& sock, std::string& result)
{
	char arr[message_length_digits + 1];
	if (!receive_stream(sock, arr, message_length_digits))
		return false;
	arr[message_length_digits] = 0;

	size_t length = 0;
	if (!(std::istringstream(arr) >> std::hex >> length))
		return false;

	result.clear();
	result.resize(length);
	if (!receive_stream(sock, result.data(), length))
		return false;

	fprintf(_log, "Received: \"%s\"\n", result.data());
	fflush(_log);

	return true;
}
bool receive_string(sf::TcpSocket& sock, std::string& result)
{
	bool initial_result = receive_string1(sock, result);
	if (!initial_result)
		return false;

	if (result.length() < 0xFFFF)
		return true;

	std::string tail;
	while (true)
	{
		bool tail_result = receive_string1(sock, tail);
		if (!tail_result)
			return false;

		if (tail.length() == 0)
			break;
		result += tail;
	}

	return true;
}

bool send_string1(sf::TcpSocket& sock, const std::string& result)
{
	fprintf(_log, "Sending: \"%s\"\n", result.data());

	char arr[message_length_digits + 1];
	int printed = snprintf(arr, sizeof(arr), "%0*zx", (int)message_length_digits, result.size());
	if (printed > message_length_digits)
		return false;

	if (sock.send(arr, strlen(arr)) != sf::Socket::Status::Done)
		return false;

	if (sock.send(result.data(), result.size()) != sf::Socket::Status::Done)
		return false;


	fprintf(_log, "Success\n");
	return true;
}
bool send_string(sf::TcpSocket& sock, const std::string& result)
{
	if (result.length() < 0xFFFF)
		return send_string1(sock, result);
	else
	{
		std::string message = result;
		while (message.length() >= 0xFFFF)
		{
			bool temp_result = send_string1(sock, message.substr(0, 0xFFFF));
			if (!temp_result)
				return false;
			message.erase(0, 0xFFFF);
		}

		return send_string1(sock, message);
	}
}



void json_stream_add_string(std::ostream& os, const std::string& s, bool comma = false, const char* quotes = "\"")
{
	os << quotes << s << quotes;
	if (comma)
		os << ',';
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

	std::string result;
	if (!receive_string(sock, result))
		result.clear();
	fprintf(_log, "\n");
	return result;
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
	return result == "{}";
}

void request_terminate(sf::TcpSocket& sock)
{
	request(sock, "terminate");
}





sf::TcpSocket dbserver;

const static std::string pictures_db = "pictures.db";
const static std::string pictures_table = "pictures";





struct tagged_thread
{
	std::jthread th;
	std::atomic_bool alive;
};

struct worker_data
{
	sf::TcpSocket client;
	std::atomic_bool* alive_ptr = nullptr;
};

void send_error(sf::TcpSocket& sock, const std::string& msg)
{
	std::ostringstream ss;
	ss << '{';

	json_stream_add_string_pair(ss, "status", "Error");
	json_stream_add_string_pair(ss, "message", msg, false);

	ss << '}';

	send_string(sock, ss.str());
}

namespace sj = simdjson;
using client_request_handler = bool (*)(worker_data*, const sj::simdjson_result<sj::dom::object>&);

bool worker_handle_request_get_picture(worker_data* data, const sj::simdjson_result<sj::dom::object>& parsed)
{
	const static std::string pictures_db = "pictures.db";
	const static std::string pictures_table = "_temp_pictures";

	auto id = parsed["request_args"].get_object()["id"];
	if (id.error())
	{
		send_error(data->client, "Picture ID not specified");
		return false;
	}

	std::ostringstream filter;
	filter << "filename == \"" << id.value_unsafe() << "\"";
	
	std::string db_response;
	if (!request_db_query(dbserver, db_response, pictures_db, pictures_table, filter.str(), "filename, size"))
	{
		send_error(data->client, "Database query error");
		return false;
	}

	sj::dom::parser p;
	auto parsed_db = p.parse(db_response).get_object();
	if (parsed_db.error())
	{
		send_error(data->client, "Database error");
		return false;
	}

	auto parsed_status = parsed_db["status"];
	auto parsed_array = parsed_db["data"].get_array();
	if (parsed_status.error() || parsed_array.error())
	{
		send_error(data->client, "Invalid database response");
		return false;
	}

	auto& array = parsed_array.value_unsafe();

	auto parsed_filename = array.at(0);
	auto parsed_size = array.at(1);
	if (parsed_filename.error() || !parsed_filename.is_string() || 
		parsed_size.error() || !parsed_size.is_uint64())
	{
		send_error(data->client, "Invalid database response");
		return false;
	}

	const auto &filename = parsed_filename.get_string().value_unsafe();
	size_t size = parsed_size.value_unsafe();

	std::filesystem::path file_path("storage");
	file_path = file_path / filename;

	std::ifstream picture_stream(file_path, std::ios::in | std::ios::binary);
	if (!picture_stream.is_open())
	{
		send_error(data->client, "File not found");
		return false;
	}

	std::vector<std::byte> pic_data;
	pic_data.resize(size);
	picture_stream.read((char*)pic_data.data(), size);

	std::ostringstream response;

	response << '{';

	json_stream_add_string_pair(response, "status", "Success");

	json_stream_add_string(response, "size");
	response << ": " << size << ',';

	json_stream_add_string_pair(response, "filename", (std::string)filename, false);

	response << '}';
	
	if (!send_string(data->client, response.str()) || 
		!send_stream(data->client, pic_data.data(), pic_data.size()))
		return false;

	return true;
}

bool worker_handle_request_upload_picture(worker_data* data, const sj::simdjson_result<sj::dom::object>& parsed)
{
	auto parsed_args = parsed["request_args"].get_object();
	if (parsed_args.error())
	{
		send_error(data->client, "Request arguments missing");
		return false;
	}

	auto parsed_filename = parsed_args["filename"].get_string();
	auto parsed_size = parsed_args["size"].get_uint64();
	auto parsed_tags = parsed_args["tags"].get_string();

	if (parsed_filename.error())
	{
		send_error(data->client, "Error parsing argument: filename");
		return false;
	}
	if (parsed_size.error())
	{
		send_error(data->client, "Error parsing argument: size");
		return false;
	}

	const auto& filename = parsed_filename.value_unsafe();
	size_t size = parsed_size.value_unsafe();

	std::vector<char> picture;
	picture.resize(size);
	if (!receive_stream(data->client, picture.data(), size))
	{
		send_error(data->client, "Error receiving picture data");
		return false;
	}

	std::ostringstream db_values;
	std::string result, tags;

	if (!parsed_tags.error())
		tags = (std::string)parsed_tags.value_unsafe();

	std::string id;
	id.resize(16);
	const char chars[] = "1234567890qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM@";

	//char id[std::size(chars)];
	//for (int i = 0; i < 64; ++i)
		//id[i] = chars[i];

	for (char& c : id)
		c = chars[mte() % std::size(chars)];

	json_stream_add_string(db_values, id, true, "\\\"");
	db_values << "0";
	db_values << ',';
	json_stream_add_string(db_values, (std::string)filename, true, "\\\"");
	db_values << size;
	db_values << ',';
	json_stream_add_string(db_values, tags, true, "\\\"");
	db_values << time(0);

	if (!request_db_add(dbserver, result, pictures_db, pictures_table, "id, user_id, filename, size, tags, created_at", db_values.str()))
		return false;
	
	std::ostringstream ss;
	if (result == "{}")
	{
		ss << '{';
		json_stream_add_string_pair(ss, "status", "Success");
		json_stream_add_string_pair(ss, "id", id, false);
		ss << "}";
	}
	else
	{
		ss << result;
	}

	if (!send_string(data->client, ss.str()))
		return false;

	std::filesystem::path file_path("storage");
	file_path = file_path / filename;

	std::ofstream(file_path, std::ios::out | std::ios::binary).write(picture.data(), picture.size());

	return true;
}

static std::unordered_map<std::string_view, client_request_handler> request_handlers = 
{
	{ "get_picture", worker_handle_request_get_picture },
	{ "upload_picture", worker_handle_request_upload_picture },
};

bool process_client(worker_data* data)
{
	std::string request;

	if (!receive_string(data->client, request))
		return false;
	
	sj::dom::parser p;
	auto parsed = p.parse(request).get_object();
	if (parsed.error())
	{
		send_error(data->client, "Invalid JSON");
		return false;
	}

	auto parsed_name_tag = parsed["request_name"].get_string();
	if (parsed_name_tag.error())
	{
		send_error(data->client, "Invalid request name");
		return false;
	}

	if (!request_handlers.contains(parsed_name_tag.value_unsafe()))
	{
		send_error(data->client, "Invalid request name");
		return false;
	}

	auto handler = request_handlers.at(parsed_name_tag.value());
	if (!handler(data, parsed))
	{
		send_error(data->client, "Server Internal Error");
		return false;
	}

	return true;
}

void log(std::ostream& os, const sf::TcpSocket& sender, auto&&... args)
{
	os << '[' << sender.getRemoteAddress() << ':' << sender.getRemotePort() << "] ";
	(os << ... << args) << '\n';
}
void log(const sf::TcpSocket& sender, auto&&... args)
{
	std::osyncstream synced_cout(std::cout);
	log(synced_cout, sender, std::forward<decltype(args)>(args)...);
}

void worker(std::unique_ptr<worker_data> data)
{
	log(data->client, "Connected");

	if (auto status = process_client(data.get()); !status)
	{
		log(data->client, "Processing error");
	}
	else
	{

	}

	log(data->client, "Disconnected");
	data->client.disconnect();
	data->alive_ptr->store(false);
}

int main()
{
	std::cout << "Looking for a DB\n";
	while (true)
	{
		auto status = dbserver.connect(DB_IP, DB_PORT, sf::seconds(2));
		if (status == sf::Socket::Done)
			break;
	}
	std::cout << "Connected to a DB at " << dbserver.getRemoteAddress() << ':' << dbserver.getRemotePort() << std::endl;
	
	if (!request_db_connection(dbserver, pictures_db))
		return 3;


	
	sf::TcpListener server;
	server.listen(SERVER_PORT);
	server.setBlocking(false);
	


	std::list<std::unique_ptr<tagged_thread>> clients;

	while (true)
	{
		auto data = std::make_unique<worker_data>();
		if (server.accept(data->client) == sf::Socket::Status::Done)
		{
			auto thread_new_data = std::make_unique<tagged_thread>();
			clients.push_back(std::move(thread_new_data));

			auto& thread_data = clients.back();

			data->alive_ptr = &thread_data->alive;

			thread_data->alive.store(true);
			thread_data->th = std::jthread(worker, std::move(data));
		}
		else
		{
			auto iter = clients.begin();
			const auto end = clients.end();

			while (iter != end)
			{
				auto& client = *iter;
				if (!client->alive)
					iter = clients.erase(iter);
				else
					++iter;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}

	return 0;
}
