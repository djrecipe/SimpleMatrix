#include "DisplayEngine.h"

DisplayEngine* engine;

static void sigintHandler(int s)
{
	engine->Stop();
	return;
}

int main(int argc, char** argv)
{
  try
  {
    Config config(argc >= 2 ? argv[1] : "/dev/null");
	engine = new DisplayEngine(config);

    signal(SIGINT, sigintHandler);
    cout << "Press Ctrl-C to quit..." << endl;

	engine->Start();
	delete engine;
  }
  catch (const exception& ex)
  {
    cerr << ex.what() << endl;
    return -1;
  }
  return 0;
}


