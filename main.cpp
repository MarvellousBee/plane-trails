#include <SFML/Graphics.hpp> // sf::
#include <cmath>             // atan2, sin, cos
#include <chrono>            // chrono::miliseconds
#include <thread>            // thread, this_thread::sleep_for
#include <iostream>          // cerr
#include <cstdlib>           // rand

// Custom colors
namespace darkColor
{
    sf::Color Red   (171,   41,     41);
    sf::Color Green (0,     153,    36);
    sf::Color Yellow(236,   247,    27);
    sf::Color Grey  (100,   100,   100);
    sf::Color Purple{255,   0,     255};
    sf::Color Cyan  {0,     255,   255};
    sf::Color Greyer(51,    51,     51);
}
namespace window_coordinates
{
    // window size
    int x{1100};
    int y{900};
    // window center
    float center_pos_x{x*.5f};
    float center_pos_y{y*.5f};
}

/// Generates a pseudo-random number
/// @param min dolna granica przedziału
/// @param max górna granica przedziału
/// @return liczba pseudlosowa
int random_number(int min, int max)
{
    return min + std::rand() % (max + 1 - min);
}

// used by 2 functions, so put in a namespace to be a little less global...
namespace pi
    {const double number = 3.14159265359;}

double rad(double degree)
{
    return degree * (pi::number / 180);
}

/// given 2 points (x,y) and assuming 1st one is the center, 
/// returns how many degreees from the x axis is y. 
/// @param a_x point a, x coordinate
/// @param a_y point a, y coordinate
/// @param b_x point b, x coordinate
/// @param b_y point b, y coordinate
/// @return y's degre from x
float angle_to_hit(float a_x, float a_y, float b_x, float b_y)
{
    float d_x = a_x - b_x;
    float d_y = a_y - b_y;

    return atan2(d_x, d_y) * 180 / pi::number;
}

class Plane : public sf::Drawable
{
public:
    // Plane sprite
    static sf::Texture plane_texture;
    sf::Sprite sprite;

    // trail variables
    static const int trail_len{600};
    int trail_iter{0};
    sf::Color color;
    sf::CircleShape trail[trail_len];
    
    // course 
    float offset{0};
    int time_until_course_change{0};

    // speed 
    float speed {2};
    int time_until_speed_change {0};

    /// @param color trail's color (Planes are always white)
    Plane(sf::Color color_) : color(color_)
    {
        // trail setup
        static const float trail_size{3.f};
        for (int i{0}; i < trail_len;i++)
        {
            trail[i].setFillColor(color);
            trail[i].setOrigin(trail_size, trail_size);
            trail[i].setRadius(trail_size);
        }
        // sprite setup
        sprite.setTexture(plane_texture);
        sprite.setOrigin(sprite.getTextureRect().width / 2, sprite.getTextureRect().height / 2);
        sprite.setPosition(0, window_coordinates::y/8);
        sprite.setScale(0.7f, 0.7f);
        sprite.rotate(90);
    }

    // move the plane 
    // when called, will move it straight forward. Distance moved is equal to the `speed` variable.
    void degree_move()
    {
        // move the plane
        double rot{sprite.getRotation() - 90};
        sprite.move(cos(rad(rot)) * speed, 
                    sin(rad(rot)) * speed);

        // move the last trail object to plane's current position
        trail[trail_iter++].setPosition(sprite.getPosition());        
        trail_iter %= trail_len;
    }
    // draw the plane, along with its trail. trail first, plane second.
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const
    {
        
        for (int i{0}; i < trail_len;i++)
                target.draw(trail[i]);
        target.draw(sprite);
        
    } 
};
// static variable
sf::Texture Plane::plane_texture;

// This function is intended to be ran as a thread.
// It handles the course and speed of each plane, as well as window's movement.
//
// Plane AI:
// Do 1) and 2) periodically and independently.
//
// 1) Check your course
//    1.1) if it's off, keep correcting it semi-randomly until the course is okay(ish).
//      use variable turning sharpness, depending on how far off-course you are.
//    1.2) if it's okay, 
//      yeet yourself off-course.
// 2) Check your distance
//    2.1) if you are close/behind the screen's left border, 
//      increase speed semi-randomly.
//    2.2) if you are close to the middle of the screen, 
//      alter your speed semi-randomly. This can make it faster or slower.
//    2.3) if you are getting close to the right border, 
//      decrease speed semi-randomly.
//
/// @param planes An array that contains all planes.
/// @param n planes' size.
/// @param window A reference to the window object.
void game_loop(Plane* planes, const int n, sf::RenderWindow& window)
{
    // This view will be moved and applied to the window.
    sf::View view(sf::Vector2f(window_coordinates::x*.5f, window_coordinates::y*.5f), sf::Vector2f(window_coordinates::x, window_coordinates::y));

    while (window.isOpen())
    {
        // iterate over every plane
        for (int i{0}; i < n;i++)
        {
            //if true, change course
            if (planes[i].time_until_course_change==0)
            {
                planes[i].time_until_course_change = random_number(5,20);

                //calculate the angle difference
                float crash_course{-angle_to_hit(planes[i].sprite.getPosition().x
                                , planes[i].sprite.getPosition().y
                                , window_coordinates::center_pos_x
                                , window_coordinates::center_pos_y)
                                };
                //float difference{std::abs(planes[i].sprite.getRotation() - crash_course)};
                float difference{planes[i].sprite.getRotation() - crash_course};

                // check the difference

                // far off course? Do a sharp turn
                if (std::abs(difference) > 60)
                {// Sharp turn
                    if (difference < 0)
                        planes[i].offset = float(random_number(1, 4))/10;
                    else
                        planes[i].offset =  float(random_number(-1, -4))/10;
                }
                // a bit off course?  Do a soft turn
                else if (std::abs(difference) > 30)
                {
                    if (difference < 0)
                        planes[i].offset = float(random_number(1, 4))/20;
                    else
                        planes[i].offset =  float(random_number(-1, -4))/20;
                }
                // no need to turn? Do a sharp turn, and stay on that course for a long while
                else
                {
                    planes[i].offset = float(random_number(-2, 2))/5;
                    planes[i].time_until_course_change = 30;
                }
            }

            // if true, change speed
            if (planes[i].time_until_speed_change==0)
            {
                planes[i].time_until_speed_change = random_number(25,50);

                // check if the plane is not too far away
                sf::Vector2f position{planes[i].sprite.getPosition()};
                float distance_to_left_border{position.x - (window_coordinates::center_pos_x-window_coordinates::x/2)};

                // in 1st 1/3 or off-screen? Falling behind, increase speed.
                if (distance_to_left_border < window_coordinates::x/3)
                    planes[i].speed += float(random_number(1, 10))/10;
                // in st 2/3 or off-screen? Falling behind, increase or decrease speed slightly.
                else if (distance_to_left_border < 2*(window_coordinates::x/3))
                   planes[i].speed += float(random_number(-3, 3))/10;
                // more than 2/3, could be off-screen? Too fast, decrease speed.
                else
                    planes[i].speed += (-float(random_number(1, 10)))/10;
                

                // make sure the plane does not go too slow
                if (planes[i].speed < 1)
                    planes[i].speed = 1;
            }
           
            
            //move planes
            planes[i].sprite.rotate(planes[i].offset);
            planes[i].degree_move();
            //reduce timers
            planes[i].time_until_course_change--;
            planes[i].time_until_speed_change--;
        }
        // move the view
        window_coordinates::center_pos_x += 2;
        view.setCenter(window_coordinates::center_pos_x, window_coordinates::center_pos_y);
        window.setView(view);
        // wait
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}


int main()
{
    // seed the RNG
    srand(time(NULL));

    // SFML window
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;
    sf::RenderWindow window(sf::VideoMode(window_coordinates::x, window_coordinates::y), "Planes", sf::Style::Default, settings);

    // load the plane texture
    if (!Plane::plane_texture.loadFromFile("Plane.png"))
    {
        std::cerr << "ERROR: File \"Plane.png\" could not be loaded.";
        return 1;
    }
    // make the array of planes
    const int num_of_planes{6};
    Plane planes[num_of_planes]
    {
        {darkColor::Red},
        {darkColor::Yellow},
        {darkColor::Purple},
        {darkColor::Green},
        {darkColor::Cyan},
        {darkColor::Grey}
    };
    for (int i{0}; i < num_of_planes;i++)
        planes[i].sprite.move(window_coordinates::y/3, (window_coordinates::x/8)*i);
    
    // start the game_loop() on another thread
    std::thread T_move_planes(game_loop, planes, num_of_planes, std::ref(window));

    // main SFML loop
    while (window.isOpen())
    {
        // check if the window is closed
        sf::Event event;
        while (window.pollEvent(event))
            if (event.type == sf::Event::Closed)
                window.close();

        // clear the window and draw planes
        window.clear(darkColor::Greyer);
        for (int i{0}; i < num_of_planes; i++)
            window.draw(planes[i]);

        window.display();
    }

    return 0;
}