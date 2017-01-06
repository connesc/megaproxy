#include <unistd.h>
#include <termios.h>
#include <mutex>
#include <condition_variable>
#include <iostream>

#define HAVE_LIBUV 1
#include <megaapi.h>

using namespace mega;
using namespace std;

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
	string megauser;
	string megapassword;
	if (argc != 1 && argc != 3) {
		cout << "Usage: " << argv[0] << " [megauser megapassword]" << endl;
		return 0;
	}

	if (argc == 1) {
		cout << "MEGA email: ";
		getline(cin, megauser);

		struct termios oldt;
		tcgetattr(STDIN_FILENO, &oldt);
		struct termios newt = oldt;
		newt.c_lflag &= ~(ECHO);
		tcsetattr(STDIN_FILENO, TCSANOW, &newt);
		cout << "MEGA password (won't be shown): ";
		getline(cin, megapassword);
		tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
		cout << endl;

	} else if (argc == 3) {
		megauser = argv[1];
		megapassword = argv[2];
	}

	MegaApi megaApi = MegaApi("CBMQALQS", (const char *)NULL, "HTTP proxy for mega.nz");
	megaApi.setLogLevel(MegaApi::LOG_LEVEL_INFO);

	// Login
	SynchronousRequestListener listener;
	megaApi.login(megauser.c_str(), megapassword.c_str(), &listener);
	listener.wait();
	if (listener.getError()->getErrorCode() != MegaError::API_OK) {
		MegaApi::log(MegaApi::LOG_LEVEL_ERROR, "Login error");
		return 0;
	}
	MegaApi::log(MegaApi::LOG_LEVEL_INFO, "Login OK. Fetching nodes");

	// Fetch nodes
	listener.reset();
	megaApi.fetchNodes(&listener);
	listener.wait();
	if (listener.getError()->getErrorCode() != MegaError::API_OK) {
		MegaApi::log(MegaApi::LOG_LEVEL_ERROR, "Error fetchning nodes");
		return 0;
	}

	MegaApi::log(MegaApi::LOG_LEVEL_INFO, "MEGA initialization complete!");
	megaApi.setLogLevel(MegaApi::LOG_LEVEL_WARNING);

	megaApi.httpServerSetRestrictedMode(MegaApi::HTTP_SERVER_ALLOW_ALL);
	megaApi.httpServerEnableFolderServer(true);
	megaApi.httpServerStart();
	pause();
}
