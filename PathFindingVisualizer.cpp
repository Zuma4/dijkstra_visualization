#include <SFML/Graphics.hpp>
#include <iostream>
#include <utility>
#include <list>
#include <array>

const int win_height = 800;
const int win_width = 800;
const int tileCount = 50;
const int tile_space_between = 1;
const int tile_size = (win_height - (tileCount * tile_space_between)) / tileCount; // sorry for the fucked up calculation

bool searchStarted = false;
bool win = false;
bool clearOnClick = false;

using Grid = std::vector<std::vector<int>>;
using GridVisual = std::vector<std::vector<sf::RectangleShape>>;
using Position = std::pair<int, int>;

enum TileType {
	empty,
	goal,
	player,
	obstacle,
	find,
	found,
};


struct Point {
	Position position = {-1, -1};
	Point* parent = nullptr;
};

void makeGrid(Grid& grid, GridVisual& gridVisual)
{
	int addy = tile_space_between;
	for (int y = 0; y < tileCount; ++y) 
	{
		int addx = tile_space_between;
		gridVisual.push_back({});
		grid.push_back({});
		for (int x = 0; x < tileCount; ++x)
		{
			grid.back().push_back(empty);
			gridVisual.back().push_back(sf::RectangleShape(sf::Vector2f{ tile_size, tile_size }));

			sf::RectangleShape& current = gridVisual.back().back();
			current.setPosition(addx, addy);

			addx += tile_size + tile_space_between;
		}

		addy += tile_size + tile_space_between;
	}

}


void drawGrid(Grid& grid, GridVisual& gridVisual, sf::RenderWindow& window)
{
	for (int y = 0; y < grid.size(); ++y)
	{
		for (int x = 0; x < grid[0].size(); ++x)
		{


			// I'm deciding the color while drawing because the color change state is very volatile
			switch (grid[y][x])
			{
			case empty: gridVisual[y][x].setFillColor(sf::Color::White); break;
			case obstacle: gridVisual[y][x].setFillColor(sf::Color(112, 128, 144)); break; // (112, 128, 144) == some kind of grey
			case player: gridVisual[y][x].setFillColor(sf::Color::Red); break;
			case goal: gridVisual[y][x].setFillColor(sf::Color::Green); break;
			case find: gridVisual[y][x].setFillColor(sf::Color::Blue); break;
			case found: gridVisual[y][x].setFillColor(sf::Color::Yellow); break;
			}

			window.draw(gridVisual[y][x]);
		}
	}

}

void clear(Grid& grid, Position& playerPos, Position& goalPos) {
	for (int y = 0; y < grid.size(); ++y) 
	{
		for (int x = 0; x < grid[0].size(); ++x)
			grid[y][x] = empty;
	}
	
	playerPos = { -1, -1 };
	goalPos = { -1, -1 };

}


void checkMouseCollision(const sf::Mouse::Button& buttonClicked, const sf::Vector2i& mousePos, Grid& grid, const GridVisual& gridVisual, Position& playerPos, Position& goalPos)
{
	sf::RectangleShape mouse{ sf::Vector2f{1,1} };
	mouse.setPosition(mousePos.x, mousePos.y);

	for (int y = 0; y < grid.size(); ++y) 
	{

		for (int x = 0; x < grid[0].size(); ++x)
		{
		
			if (mouse.getGlobalBounds().intersects(gridVisual[y][x].getGlobalBounds()))
			{
				switch (buttonClicked)
				{
				case sf::Mouse::Left:
					grid[y][x] = obstacle;
					break;
				case sf::Mouse::Right:
					grid[y][x] = empty;
					break;
				case sf::Mouse::XButton1:
					if (playerPos.first != -1)
						grid[playerPos.first][playerPos.second] = empty;
					grid[y][x] = player;
					playerPos = { y, x };
					break;
				case sf::Mouse::XButton2:
					if (goalPos.first != -1)
						grid[goalPos.first][goalPos.second] = empty;
					grid[y][x] = goal;
					goalPos = { y, x };
					break;
				}

				if (playerPos.first == y && playerPos.second == x && buttonClicked != sf::Mouse::XButton1)
					playerPos = { -1, -1 };
				else if (goalPos.first == y && goalPos.second == x && buttonClicked != sf::Mouse::XButton2)
					goalPos = { -1, -1 };
			}
		}
	}
}

void markWinningPath(Point winPoint, Grid& grid) {
	while (winPoint.parent != nullptr) {
		grid[winPoint.position.first][winPoint.position.second] = found;
		winPoint = *winPoint.parent;
	}

	for (int y = 0; y < grid.size(); ++y)
	{
		for (int x = 0; x < grid[0].size(); ++x)
			if (grid[y][x] == find)
				grid[y][x] = empty;
	}
}

bool dijkstra(Grid& grid, std::list<std::pair<Point, bool>>& consids, Point& winPoint)
{
	std::array<std::pair<int, int>, 4> directions{ {{1, 0}, {-1, 0}, {0, 1}, {0, -1}} };
	int consSize = consids.size();
	int i = 0;

	for (auto& point : consids)
	{
		if (point.second == true)
			continue;

		int ind_y = point.first.position.first;
		int ind_x = point.first.position.second;

		for (int dir = 0; dir < directions.size(); ++dir)
		{
			int next_ind_y = ind_y + directions[dir].first;
			int next_ind_x = ind_x + directions[dir].second;

			if (next_ind_y >= 0 && next_ind_y < grid.size() &&
				next_ind_x >= 0 && next_ind_x < grid[0].size() &&
				grid[next_ind_y][next_ind_x] != find &&
				grid[next_ind_y][next_ind_x] != obstacle &&
				grid[next_ind_y][next_ind_x] != player)
			{
				point.second = true; // we don't want to consider this one anymore
				consids.push_back({ Point{ {next_ind_y, next_ind_x}, &point.first }, false });
				if(grid[next_ind_y][next_ind_x] == goal)
				{
					winPoint = point.first;
					return true;
				}

				grid[next_ind_y][next_ind_x] = find;
			}
		}

		if (i == consSize - 1)
			break;

		++i;

	}

	return false;

}


int main() 
{
	sf::RenderWindow window(sf::VideoMode(win_width, win_height), "Path Finding Visualization");
	window.setFramerateLimit(60);

	sf::Clock searchClock;

	Grid grid;
	GridVisual gridVisual;
	makeGrid(grid, gridVisual);

	Position playerPos{-1, -1};
	Position goalPos{-1, -1};
	Point winPoint{};

	std::list<std::pair<Point, bool>> consids;


	while (window.isOpen()) 
	{


		if (!searchStarted)
		{
			bool clicked = false;
			if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
			{
				checkMouseCollision(sf::Mouse::Left, sf::Mouse::getPosition(window), grid, gridVisual, playerPos, goalPos);
				clicked = true;
			}
			else if (sf::Mouse::isButtonPressed(sf::Mouse::Right))
			{
				checkMouseCollision(sf::Mouse::Right, sf::Mouse::getPosition(window), grid, gridVisual, playerPos, goalPos);
				clicked = true;
			}
			else if (sf::Mouse::isButtonPressed(sf::Mouse::XButton1))
			{
				checkMouseCollision(sf::Mouse::XButton1, sf::Mouse::getPosition(window), grid, gridVisual, playerPos, goalPos);
				clicked = true;
			}

			else if (sf::Mouse::isButtonPressed(sf::Mouse::XButton2))
			{
				checkMouseCollision(sf::Mouse::XButton2, sf::Mouse::getPosition(window), grid, gridVisual, playerPos, goalPos);
				clicked = true;
			}

			if (clicked && clearOnClick)
			{
				clear(grid, playerPos, goalPos);
				clearOnClick = false;
			}

		}

		

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Enter))
		{
			if (!searchStarted && playerPos.first != -1 && goalPos.first != -1)
			{
				consids.clear();
				consids.push_back({ Point{ playerPos, nullptr }, false });
				searchClock.restart();
				searchStarted = true;
			}
		}

		if (searchStarted && searchClock.getElapsedTime().asSeconds() > 0.2f)
		{
			bool result = dijkstra(grid, consids, winPoint);
			if (result)
			{
				markWinningPath(winPoint, grid);
				searchStarted = false;
				clearOnClick = true;
			}

			searchClock.restart();
		}


		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
		}


		window.clear();
		drawGrid(grid, gridVisual, window);
		window.display();

	}

	return 0;
}

// fix markWinningPath()