#include <iostream>

#include <mango/image/image.hpp>

int main()
{
	std::cout << "decoding ..\\..\\testorig.jpg " << std::endl;

	mango::Bitmap bmp(" ..\\..\\testorig.jpg");

	std::cout << "saving as PNG ..." << std::endl;
	bmp.save(".\\asPng.png");

	std::cout << "DONE!" << std::endl;

	system("pause");

	return 0;
}