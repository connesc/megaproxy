#include <unistd.h>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <fstream>
#include <boost/program_options.hpp>

#define HAVE_LIBUV 1
#include <megaapi.h>

using namespace mega;
using namespace std;

namespace po = boost::program_options;

class SynchronousRequestListener : public MegaRequestListener {
	public:
		~SynchronousRequestListener() {
			delete request;
			delete error;
		}

		void onRequestFinish(MegaApi *, MegaRequest *request, MegaError *error) {
			this->error = error->copy();
			this->request = request->copy();

			{
				unique_lock<mutex> lock(m);
				notified = true;
			}
			cv.notify_all();
		}

		void wait() {
			unique_lock<mutex> lock(m);
			cv.wait(lock, [this]{return notified;});
		}

		void reset() {
			delete request;
			delete error;
			request = NULL;
			error = NULL;
			notified = false;
		}

		const MegaRequest *getRequest() const {
			return request;
		}

		const MegaError *getError() const {
			return error;
		}

	private:
		bool notified = false;
		const MegaError *error = NULL;
		const MegaRequest *request = NULL;
		condition_variable cv;
		mutex m;
};

int main(int argc, char *argv[]) {
	// Parse options
	string username;
	string password;

	bool localOnly;
	int port;

	bool fileServer;
	bool folderServer;
	bool subtitlesSupport;
	int maxBufferSize;
	int maxOutputSize;

	po::options_description generic("Generic options");
	generic.add_options()
		("help,h", "produce help message")
		("config,c", po::value<string>(), "configuration file")
	;

	po::options_description config("Configuration");
	config.add_options()
		("user,u", po::value<string>(&username)->required(), "mega.nz username (required)")
		("pass,p", po::value<string>(&password)->required(), "mega.nz password (required)")
		("local-only", po::value<bool>(&localOnly)->default_value(true), "listen on 127.0.0.1 only")
		("port", po::value<int>(&port)->default_value(4443), "listening port")
		("files", po::value<bool>(&fileServer)->default_value(true), "allow to serve files")
		("folders", po::value<bool>(&folderServer)->default_value(true), "allow to serve folders")
		("subtitles", po::value<bool>(&subtitlesSupport)->default_value(false), "enable subtitles support")
		("buffer", po::value<int>(&maxBufferSize)->default_value(0), "maximum buffer size (in bytes)")
		("output", po::value<int>(&maxOutputSize)->default_value(0), "maximum output size (in bytes)")
	;

	po::options_description desc;
	desc.add(generic).add(config);

	try {
		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);

		if (vm.count("help")) {
			cout << "Usage: " << argv[0] << " [options]" << endl << desc << endl;
			return 0;
		}

		if (vm.count("config")) {
			const string &path = vm["config"].as<string>();
			ifstream file(path.c_str());

			if (!file) {
				cerr << "Cannot open config file: " << path << endl;
			} else {
				po::store(po::parse_config_file(file, config), vm);
			}
		}
		po::notify(vm);

	} catch (exception& e) {
		cerr << "Invalid options: " << e.what() << endl << desc << endl;
		return 1;
	}

	// Initiliaze the SDK
	MegaApi megaApi = MegaApi("CBMQALQS", (const char *)NULL, "HTTP proxy for mega.nz");
	megaApi.setLogLevel(MegaApi::LOG_LEVEL_INFO);

	// Login
	SynchronousRequestListener listener;
	megaApi.login(username.c_str(), password.c_str(), &listener);
	listener.wait();
	if (listener.getError()->getErrorCode() != MegaError::API_OK) {
		MegaApi::log(MegaApi::LOG_LEVEL_ERROR, "Login error");
		return 2;
	}
	MegaApi::log(MegaApi::LOG_LEVEL_INFO, "Login OK. Fetching nodes");

	// Fetch nodes
	listener.reset();
	megaApi.fetchNodes(&listener);
	listener.wait();
	if (listener.getError()->getErrorCode() != MegaError::API_OK) {
		MegaApi::log(MegaApi::LOG_LEVEL_ERROR, "Error fetching nodes");
		return 3;
	}

	// Setup the HTTP server
	megaApi.httpServerEnableFileServer(fileServer);
	megaApi.httpServerEnableFolderServer(folderServer);
	megaApi.httpServerSetRestrictedMode(MegaApi::HTTP_SERVER_ALLOW_ALL);
	megaApi.httpServerEnableSubtitlesSupport(subtitlesSupport);
	megaApi.httpServerSetMaxBufferSize(maxBufferSize);
	megaApi.httpServerSetMaxOutputSize(maxOutputSize);
	megaApi.httpServerStart(localOnly, port);

	MegaApi::log(MegaApi::LOG_LEVEL_INFO, "MEGA initialization complete!");
	megaApi.setLogLevel(MegaApi::LOG_LEVEL_WARNING);
	pause();
}
