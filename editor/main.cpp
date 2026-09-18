#define STB_IMAGE_IMPLEMENTATION

#include "src/EditorApp.h"

int main()
{
	Window::Desc desc;
	desc.name = "Phoenix editor";
	desc.resizable = true;

	EditorApp app{desc};
	app.Run();

	return 0;
}