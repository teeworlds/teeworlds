/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <iostream>
#include <fstream>

using namespace std;

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		cout << "Usage: bla <infile.tga>" << endl;
		return -1;
	}

	ifstream file(argv[1]);

	if (!file)
	{
		cout << "No such file..." << endl;
		return -1;
	}

	unsigned short headers[9];

	file.read((char *)headers, 18);

	int width = headers[6];
	int height = headers[7];

	const int charsx = 16;
	const int charsy = 16;
	const int charWidth = width / charsx;
	const int charHeight = height / charsy;

	char *data = new char[width * height * 4];

	file.read(data, width * height * 4);
	
    int startTable[256] = {0};
	int endTable[256] = {0};

    for (int i = 0; i < charsy; i++)
        for (int j = 0; j < charsx; j++)
        {
            bool done = false;

            for (int x = 0; x < charWidth && !done; ++x)
                for (int y = charHeight - 1; y >= 0; --y)
                {   
                    // check if alpha is != 0 
                    int tempX = j * charWidth + x;
                    int tempY = i * charHeight + y;
                    
                    int coordIndex = tempX + tempY * width;
                    
                    if (data[4 * coordIndex + 3] != 0)
                    {   
                        // if it is, save the x-coord to table and go to next character
                        startTable[j + i * charsx] = x;
                        done = true;
                    }
                }


			done = false;
            for (int x = charWidth - 1; x >= 0 && !done; --x)
                for (int y = charHeight - 1; y >= 0; --y)
                {
                    // check if alpha is != 0
                    int tempX = j * charWidth + x;
                    int tempY = i * charHeight + y;

                    int coordIndex = tempX + tempY * width;

                    if (data[4 * coordIndex + 3] != 0)
                    {
                        // if it is, save the x-coord to table and go to next character
                        endTable[j + i * charsx] = x;
                        done = true;
                    }
                }
        }

    delete[] data;

    cout << "float CharStartTable[] =" << endl << '{' << endl << '\t';

    for (int i = 0; i < 256; i++)
    {
        cout << startTable[i] / float(charWidth) << ", ";
        if (!((i + 1) % 16))
            cout << endl << '\t';
    }

    cout << endl << "};" << endl;

    cout << "float CharEndTable[] =" << endl << '{' << endl << '\t';

    for (int i = 0; i < 256; i++)
    {
        cout << endTable[i] / float(charWidth) << ", ";
        if (!((i + 1) % 16))
            cout << endl << '\t';
    }

    cout << endl << "};" << endl;

        
    cout << charWidth << 'x' << charHeight << endl;
}
