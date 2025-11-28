#pragma once
class GranularSynthUI
{
public:

    GranularSynthUI();
    ~GranularSynthUI();

    void gui();

    bool showWindow = false;

private:
    // Pimpl (hide code in cpp file to save compile time and better encapsulate the class)
    class Impl; // Forward declaration of the implementation class
    Impl* pimpl_; // Raw pointer to the implementation
};

