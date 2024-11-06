#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <list>
#include <stack>
#include <queue>
#include <algorithm>
#include <string>
#include <iterator>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <functional>
#include <random>
#include <chrono>
#include <thread>
#include <optional>

using namespace std;

// Player class to hold player information
class Player {
public:
    string name; // Name of the player
    int money; // Amount of money player has
    int position; // Player's current position on the board (0-39)
    bool inJail; // Flag indicating if the player is in Jail
    int jailTurns; // Number of turns the player has been in Jail
    bool bankrupt; // Flag indicating if the player is bankrupt
    unordered_set<string> propertiesOwned; // Set of property names owned by the player
    unordered_map<string, int> propertyUpgrades; // Tracks upgrades (houses/hotels) on properties owned by the player
    bool isAI; // Flag to indicate if the player is an AI

    // Constructor to initialize player details
    Player(string name, int money = 1500, int position = 0, bool isAI = false)
        : name(name), money(money), position(position), inJail(false), jailTurns(0), bankrupt(false), isAI(isAI) {}
};

// Struct for bids in auctions
struct Bid {
    int amount;
    string bidderName;

    bool operator<(const Bid& other) const {
        return amount < other.amount; // For max-heap
    }
};

// Board class to manage game operations
class Board {
public:
    // Various data structures to represent board, players, and game state
    unordered_map<int, string> properties; // Maps position numbers to property names
    list<Player> players; // List of players in the game
    stack<string> communityChest; // Stack representing community chest cards
    queue<string> chance; // Queue representing chance cards
    unordered_set<string> ownedProperties; // Set of all owned properties in the game
    unordered_map<string, string> propertyOwners; // Maps property names to their owners
    unordered_map<string, int> rentPrices; // Maps property names to their base rent prices
    unordered_map<string, bool> mortgagedProperties; // Tracks whether a property is mortgaged or not
    unordered_map<int, string> specialSpaces; // Maps board positions to special spaces (e.g., Go, Jail)
    const int upgradeCost = 100; // Cost to upgrade a property
    const int rentMultiplier = 2; // Rent multiplier per upgrade level
    std::random_device rd; // Random device for random number generation
    std::mt19937 rng; // Mersenne Twister random number generator

    // Constructor to initialize the board and game elements
    Board() : rng(rd()) {
        // Initialize properties with their positions and names
        properties = {
            {1, "Mediterranean Avenue"}, {3, "Baltic Avenue"},
            {5, "Reading Railroad"}, {6, "Oriental Avenue"},
            {8, "Vermont Avenue"}, {9, "Connecticut Avenue"},
            {11, "St. Charles Place"}, {13, "States Avenue"},
            {14, "Virginia Avenue"}, {16, "St. James Place"},
            {18, "Tennessee Avenue"}, {19, "New York Avenue"},
            {21, "Kentucky Avenue"}, {23, "Indiana Avenue"},
            {24, "Illinois Avenue"}, {26, "Atlantic Avenue"},
            {27, "Ventnor Avenue"}, {29, "Marvin Gardens"},
            {31, "Pacific Avenue"}, {32, "North Carolina Avenue"},
            {34, "Pennsylvania Avenue"}, {37, "Park Place"},
            {39, "Boardwalk"}
        };

        // Set base rent prices for all properties
        for (const auto& property : properties) {
            rentPrices[property.second] = 50; // Simplified rent price for all properties
        }

        // Initialize community chest cards
        list<string> communityChestCards = {
            "Bank error in your favor. Collect $200.",
            "Doctor's fees. Pay $50.",
            "From sale of stock you get $50.",
            "Get Out of Jail Free.",
            "Go to Jail. Go directly to jail, do not pass Go, do not collect $200.",
            "Holiday Fund matures. Receive $100.",
            "Income tax refund. Collect $20.",
            "Life insurance matures. Collect $100.",
            "Pay hospital fees of $100.",
            "Pay school fees of $150.",
            "Receive $25 consultancy fee.",
            "You have won second prize in a beauty contest. Collect $10.",
            "You inherit $100."
        };

        // Shuffle community chest cards
        communityChestCards = shuffleList(communityChestCards);

        // Push shuffled cards into the stack
        for (const auto& card : communityChestCards) {
            communityChest.push(card);
        }

        // Initialize chance cards
        list<string> chanceCards = {
            "Advance to Go (Collect $200).",
            "Advance to Illinois Ave. If you pass Go, collect $200.",
            "Advance to St. Charles Place. If you pass Go, collect $200.",
            "Bank pays you dividend of $50.",
            "Get out of Jail Free.",
            "Go Back 3 Spaces.",
            "Go directly to Jail. Do not pass Go, do not collect $200.",
            "Make general repairs on all your property. For each house pay $25.",
            "Pay poor tax of $15.",
            "Take a trip to Reading Railroad. If you pass Go, collect $200.",
            "Take a walk on the Boardwalk. Advance token to Boardwalk.",
            "You have been elected Chairman of the Board. Pay each player $50.",
            "Your building loan matures. Collect $150."
        };

        // Shuffle chance cards
        chanceCards = shuffleList(chanceCards);

        // Enqueue shuffled cards into the queue
        for (const auto& card : chanceCards) {
            chance.push(card);
        }

        // Initialize special spaces on the board (e.g., Go, Jail, etc.)
        specialSpaces = {
            {0, "Go"},
            {4, "Income Tax"},
            {10, "Jail"},
            {20, "Free Parking"},
            {30, "Go to Jail"},
            {38, "Luxury Tax"}
        };
    }

    // Function to shuffle a list
    template <typename T>
    list<T> shuffleList(list<T>& inputList) {
        // Convert list to vector for shuffling
        list<T> shuffledList;
        list<T> tempList = inputList;
        std::list<T> tempShuffledList;

        // Copy list to a vector for shuffling
        list<T> tempCopyList(tempList);
        vector<T> tempVector;
        std::copy(tempCopyList.begin(), tempCopyList.end(), back_inserter(tempVector));

        // Shuffle the vector
        std::shuffle(tempVector.begin(), tempVector.end(), rng);

        // Copy back to a list
        std::copy(tempVector.begin(), tempVector.end(), back_inserter(tempShuffledList));

        return tempShuffledList;
    }

    // Add a player to the game by creating a Player object and adding it to the list of players
    void addPlayer(const string& playerName, bool isAI = false) {
        players.emplace_back(playerName, 1500, 0, isAI);
    }

    // Function to simulate a player's turn
    void playTurn(Player& player) {
        // If player is bankrupt, skip their turn
        if (player.bankrupt) {
            return;
        }

        // If player is in jail, handle their jail turn
        if (player.inJail) {
            handleJailTurn(player);
            return;
        }

        // Roll a dice to determine movement (1 to 6)
        std::uniform_int_distribution<int> dist(1, 6);
        int roll = dist(rng);
        player.position = (player.position + roll) % 40; // Update player position, board has 40 spaces

        // Display board visualization
        displayBoard();

        // Check if player landed on a special space
        auto specialSpaceIt = specialSpaces.find(player.position);
        if (specialSpaceIt != specialSpaces.end()) {
            handleSpecialSpace(player, player.position);
            return;
        }

        // Handle properties landing
        auto propertyIt = properties.find(player.position);
        if (propertyIt != properties.end()) {
            string propertyName = propertyIt->second;
            cout << player.name << " rolled a " << roll << " and landed on " << propertyName << endl;

            // If property is not owned by any player
            if (ownedProperties.find(propertyName) == ownedProperties.end()) {
                if (player.isAI) {
                    // AI Decision to buy property
                    if (shouldAIBuyProperty(player, propertyName)) {
                        player.money -= 100;
                        ownedProperties.insert(propertyName);
                        propertyOwners[propertyName] = player.name;
                        player.propertiesOwned.insert(propertyName);
                        player.propertyUpgrades[propertyName] = 0;
                        cout << player.name << " (AI) bought " << propertyName << endl;
                    } else {
                        cout << player.name << " (AI) decided not to buy " << propertyName << "." << endl;
                    }
                } else {
                    // Offer to buy the property for human players
                    cout << propertyName << " is available for purchase." << endl;
                    char choice;
                    cout << "Do you want to buy it? (y/n): ";
                    cin >> choice;
                    if (choice == 'y') {
                        player.money -= 100; // Assume all properties cost $100 for simplicity
                        ownedProperties.insert(propertyName);
                        propertyOwners[propertyName] = player.name;
                        player.propertiesOwned.insert(propertyName);
                        player.propertyUpgrades[propertyName] = 0; // No upgrades initially
                        cout << player.name << " bought " << propertyName << endl;
                    } else {
                        // Start auction if player does not want to buy
                        auctionProperty(propertyName);
                    }
                }
            } else {
                // Property is owned by another player
                if (propertyOwners[propertyName] != player.name) {
                    // If the property is not mortgaged, pay rent
                    if (!mortgagedProperties[propertyName]) {
                        int rent = rentPrices[propertyName] * (1 + player.propertyUpgrades[propertyName] * rentMultiplier);
                        cout << propertyName << " is owned by " << propertyOwners[propertyName] << ". You must pay rent of $" << rent << "." << endl;
                        player.money -= rent;
                        if (player.money < 0) {
                            handleBankruptcy(player);
                            return;
                        }
                        // Pay rent to the property owner
                        auto ownerPlayer = findPlayer(propertyOwners[propertyName]);
                        if (ownerPlayer) {
                            ownerPlayer.value().get().money += rent;
                        }
                        cout << player.name << " paid $" << rent << " in rent to " << propertyOwners[propertyName] << endl;
                    } else {
                        // Property is mortgaged, no rent is paid
                        cout << propertyName << " is mortgaged. No rent is due." << endl;
                    }
                } else {
                    // Player landed on their own property
                    cout << propertyName << " is already owned by you." << endl;
                }
            }
        } else {
            // Player landed on an empty space (not a property or special space)
            cout << player.name << " rolled a " << roll << " and landed on an empty space." << endl;
        }

        // Random chance to draw a community chest card (1 in 5 chance)
        if (dist(rng) == 1) {
            drawCommunityChest(player);
        }

        // Offer player the option to upgrade one of their properties
        if (!player.isAI) {
            upgradeProperty(player);
        }
    }

    // Smarter AI decision-making to decide whether to buy a property
    bool shouldAIBuyProperty(const Player& player, const string& propertyName) {
        if (player.money < 150) {
            return false; // AI won't buy if low on money
        }
        if (propertyName == "Boardwalk" || propertyName == "Park Place") {
            return true; // AI always buys premium properties
        }
        return (rand() % 2 == 0); // Random decision for other properties
    }

    // Display the current state of the board
    void displayBoard() const {
        list<string> board(40, "[ ]");

        // Use iterators to place players on the board
        for (const auto& player : players) {
            if (!player.bankrupt) {
                auto it = board.begin();
                advance(it, player.position);
                *it = "[" + player.name.substr(0, 1) + "]";
            }
        }

        cout << "\nBoard State:\n";
        int count = 0;
        for (const auto& space : board) {
            cout << space;
            count++;
            if (count % 10 == 0) {
                cout << endl;
            }
        }
        cout << endl;
    }

    // Handle actions for when a player lands on a special space (e.g., Go, Jail)
    void handleSpecialSpace(Player& player, int position) {
        string spaceName = specialSpaces[position];
        cout << player.name << " landed on " << spaceName << endl;
        if (spaceName == "Go") {
            player.money += 200; // Player collects $200 for landing on or passing Go
            cout << player.name << " collects $200 for landing on Go." << endl;
        } else if (spaceName == "Income Tax") {
            // Player pays either 10% of their total money or $200, whichever is lower
            int tax = min(200, static_cast<int>(player.money * 0.1));
            player.money -= tax;
            if (player.money < 0) {
                handleBankruptcy(player);
                return;
            }
            cout << player.name << " pays Income Tax of $" << tax << endl;
        } else if (spaceName == "Go to Jail") {
            // Player is sent to Jail
            player.inJail = true;
            player.position = 10; // Jail position is 10
            cout << player.name << " is sent to Jail!" << endl;
        } else if (spaceName == "Luxury Tax") {
            // Player pays a luxury tax of $100
            player.money -= 100;
            if (player.money < 0) {
                handleBankruptcy(player);
                return;
            }
            cout << player.name << " pays Luxury Tax of $100." << endl;
        } else if (spaceName == "Free Parking") {
            // Free Parking does nothing, it's just a resting space
            cout << player.name << " is on Free Parking. Nothing happens." << endl;
        } else if (spaceName == "Jail") {
            // Player is just visiting Jail, not actually in Jail
            cout << player.name << " is just visiting Jail." << endl;
        }
    }

    // Handle the player's turn when they are in Jail
    void handleJailTurn(Player& player) {
        if (player.jailTurns < 3) {
            // Player attempts to roll doubles to get out of Jail
            cout << player.name << " is in jail. Attempting to roll a double to get out..." << endl;
            std::uniform_int_distribution<int> dist(1, 6);
            int roll1 = dist(rng);
            int roll2 = dist(rng);
            cout << "Rolled: " << roll1 << " and " << roll2 << endl;
            if (roll1 == roll2) {
                // Player rolled a double and gets out of Jail
                cout << player.name << " rolled a double and is free from jail!" << endl;
                player.inJail = false;
                player.jailTurns = 0;
            } else {
                // Player did not roll a double, must stay in Jail
                player.jailTurns++;
                cout << player.name << " did not roll a double and must stay in jail." << endl;
            }
        } else {
            // Player has served 3 turns and is automatically released
            cout << player.name << " has served 3 turns in jail and is now free." << endl;
            player.inJail = false;
            player.jailTurns = 0;
        }
    }

    // Allow a player to mortgage a property for cash
    void mortgageProperty(Player& player) {
        // If player is bankrupt, they cannot mortgage any properties
        if (player.bankrupt) return;

        // Display properties that the player owns which are not mortgaged
        cout << "Properties you own: " << endl;
        for (const auto& property : player.propertiesOwned) {
            if (!mortgagedProperties[property]) {
                cout << property << endl;
            }
        }

        // Ask the player which property they want to mortgage
        cout << "Enter the property you want to mortgage: ";
        string propertyName;
        cin.ignore();
        getline(cin, propertyName);

        // Check if the player owns the property and if it is not already mortgaged
        if (player.propertiesOwned.find(propertyName) != player.propertiesOwned.end() && !mortgagedProperties[propertyName]) {
            mortgagedProperties[propertyName] = true; // Mark property as mortgaged
            player.money += 50; // Mortgage value is $50 for simplicity
            cout << propertyName << " has been mortgaged. You received $50." << endl;
        } else {
            // Invalid property or already mortgaged
            cout << "Invalid property or already mortgaged." << endl;
        }
    }

    // Allow players to trade properties
    void tradeProperty(Player& player) {
        if (player.bankrupt) return; // Skip if player is bankrupt

        // Get the name of the player they want to trade with
        cout << "Enter the name of the player you want to trade with: ";
        string otherPlayerName;
        cin.ignore();
        getline(cin, otherPlayerName);

        auto maybeOtherPlayer = findPlayer(otherPlayerName);
        if (!maybeOtherPlayer) {
            cout << "Player not found." << endl;
            return;
        }

        Player& otherPlayer = maybeOtherPlayer.value().get();

        // Display the properties owned by the player initiating the trade
        cout << "Properties you own: " << endl;
        for (const auto& property : player.propertiesOwned) {
            cout << property << endl;
        }

        // Ask the player to choose a property to trade
        cout << "Enter the property you want to trade: ";
        string playerProperty;
        getline(cin, playerProperty);

        // Validate if the player owns the property
        if (player.propertiesOwned.find(playerProperty) == player.propertiesOwned.end()) {
            cout << "You do not own this property." << endl;
            return;
        }

        // Display the properties owned by the other player
        cout << otherPlayer.name << "'s properties: " << endl;
        for (const auto& property : otherPlayer.propertiesOwned) {
            cout << property << endl;
        }

        // Ask for the property the player wants in return
        cout << "Enter the property you want in return: ";
        string otherProperty;
        getline(cin, otherProperty);

        // Validate if the other player owns the property
        if (otherPlayer.propertiesOwned.find(otherProperty) == otherPlayer.propertiesOwned.end()) {
            cout << "The other player does not own this property." << endl;
            return;
        }

        // Execute the trade: exchange properties between the players
        player.propertiesOwned.erase(playerProperty);
        otherPlayer.propertiesOwned.erase(otherProperty);
        player.propertiesOwned.insert(otherProperty);
        otherPlayer.propertiesOwned.insert(playerProperty);
        propertyOwners[playerProperty] = otherPlayer.name;
        propertyOwners[otherProperty] = player.name;
        cout << "Trade successful! " << player.name << " traded " << playerProperty << " for " << otherProperty << " with " << otherPlayer.name << endl;
    }

    // Start an auction for a property
    void auctionProperty(const string& propertyName, int startingBid = 10, int bidIncrement = 5) {
        cout << "Starting auction for " << propertyName << "! Starting bid is $" << startingBid << " with bid increment of $" << bidIncrement << "." << endl;

        priority_queue<Bid> bidQueue;

        // Collect bids from players
        for (auto& player : players) {
            if (player.bankrupt) continue;

            int bid = 0;
            if (player.isAI) {
                bid = (rand() % 2 == 0) ? startingBid : 0;
                cout << player.name << " (AI) bids: " << (bid > 0 ? to_string(bid) : "Pass") << endl;
            } else {
                cout << player.name << ", enter your bid (or 0 to pass, must be at least $" << startingBid << "): ";
                cin >> bid;
            }

            if (bid >= startingBid) {
                bidQueue.push({bid, player.name});
                startingBid = bid + bidIncrement;
            }
        }

        // Determine the winner
        if (!bidQueue.empty()) {
            Bid highestBid = bidQueue.top();
            cout << highestBid.bidderName << " wins the auction for " << propertyName << " with a bid of $" << highestBid.amount << "!" << endl;

            // Update ownership
            ownedProperties.insert(propertyName);
            propertyOwners[propertyName] = highestBid.bidderName;

            // Deduct money and add property to the winner
            auto winnerIt = std::find_if(players.begin(), players.end(),
                [&highestBid](const Player& p) { return p.name == highestBid.bidderName; });
            if (winnerIt != players.end()) {
                winnerIt->propertiesOwned.insert(propertyName);
                winnerIt->propertyUpgrades[propertyName] = 0;
                winnerIt->money -= highestBid.amount;
                if (winnerIt->money < 0) {
                    handleBankruptcy(*winnerIt);
                }
            }
        } else {
            cout << "No bids were placed for the property." << endl;
        }
    }

    // Draw a community chest card and apply its effect
    void drawCommunityChest(Player& player) {
        // If there are no community chest cards left, reshuffle
        if (communityChest.empty()) {
            cout << "No community chest cards left. Reshuffling..." << endl;
            // Reinitialize and reshuffle
            list<string> communityChestCards = {
                "Bank error in your favor. Collect $200.",
                "Doctor's fees. Pay $50.",
                "From sale of stock you get $50.",
                "Get Out of Jail Free.",
                "Go to Jail. Go directly to jail, do not pass Go, do not collect $200.",
                "Holiday Fund matures. Receive $100.",
                "Income tax refund. Collect $20.",
                "Life insurance matures. Collect $100.",
                "Pay hospital fees of $100.",
                "Pay school fees of $150.",
                "Receive $25 consultancy fee.",
                "You have won second prize in a beauty contest. Collect $10.",
                "You inherit $100."
            };
            communityChestCards = shuffleList(communityChestCards);
            for (const auto& card : communityChestCards) {
                communityChest.push(card);
            }
        }

        // Draw the top card from the community chest stack
        string card = communityChest.top();
        communityChest.pop();
        cout << "Community Chest: " << card << endl;

        // Determine the effect of the drawn card
        if (card.find("Collect $") != string::npos) {
            size_t pos = card.find("$") + 1;
            int amount = stoi(card.substr(pos));
            player.money += amount;
            cout << player.name << " collects $" << amount << " from Community Chest." << endl;
        } else if (card.find("Pay $") != string::npos) {
            size_t pos = card.find("$") + 1;
            int amount = stoi(card.substr(pos));
            player.money -= amount;
            if (player.money < 0) {
                handleBankruptcy(player);
                return;
            }
            cout << player.name << " pays $" << amount << " for Community Chest card." << endl;
        } else if (card == "Go to Jail. Go directly to jail, do not pass Go, do not collect $200.") {
            player.inJail = true;
            player.position = 10;
            cout << player.name << " is sent to Jail!" << endl;
        } else if (card == "Get Out of Jail Free.") {
            // Implement logic to give player a get out of jail free card if desired
            cout << player.name << " received a Get Out of Jail Free card." << endl;
        }
    }

    // Allow a player to upgrade a property
    void upgradeProperty(Player& player) {
        if (player.bankrupt) return;

        cout << "Do you want to upgrade a property? (y/n): ";
        char choice;
        cin >> choice;
        if (choice == 'y') {
            cout << "Properties you can upgrade: " << endl;
            for (const auto& property : player.propertiesOwned) {
                if (!mortgagedProperties[property]) {
                    cout << property << " (Current Upgrades: " << player.propertyUpgrades[property] << ")" << endl;
                }
            }
            cout << "Enter the property you want to upgrade: ";
            string propertyName;
            cin.ignore();
            getline(cin, propertyName);
            if (player.propertiesOwned.find(propertyName) != player.propertiesOwned.end() && !mortgagedProperties[propertyName]) {
                if (player.money >= upgradeCost) {
                    player.money -= upgradeCost;
                    player.propertyUpgrades[propertyName]++;
                    cout << propertyName << " has been upgraded. Total upgrades: " << player.propertyUpgrades[propertyName] << endl;
                } else {
                    cout << "You do not have enough money to upgrade this property." << endl;
                }
            } else {
                cout << "Invalid property or property is mortgaged." << endl;
            }
        }
    }

    // Handle bankruptcy of a player
    void handleBankruptcy(Player& player) {
        cout << player.name << " is bankrupt! All properties are now up for auction." << endl;
        player.bankrupt = true;
        player.money = 0;

        // Auction off all player's properties
        for (const auto& property : player.propertiesOwned) {
            auctionProperty(property);
        }
        player.propertiesOwned.clear();
    }

    // Find a player by name
    optional<reference_wrapper<Player>> findPlayer(const string& name) {
        auto it = std::find_if(players.begin(), players.end(),
            [&name](const Player& p) { return p.name == name; });
        if (it != players.end()) {
            return *it;
        } else {
            return nullopt;
        }
    }

    // Display all players and their current status
    void displayPlayersStatus() const {
        cout << "\nCurrent Players Status:\n";
        for (const auto& player : players) {
            cout << player.name << " - Money: $" << player.money << ", Position: " << player.position;
            if (player.inJail) {
                cout << " (In Jail)";
            }
            cout << ", Bankrupt: " << (player.bankrupt ? "Yes" : "No") << endl;
        }
    }

    // Calculate and display total wealth of each player
    void displayTotalWealth() const {
        cout << "\nTotal Wealth of Each Player:\n";
        for (const auto& player : players) {
            int totalWealth = player.money;
            totalWealth += std::accumulate(player.propertiesOwned.begin(), player.propertiesOwned.end(), 0,
                [this](int sum, const string& property) {
                    return sum + 100; // Assuming each property is worth $100
                });
            cout << player.name << " - Total Wealth: $" << totalWealth << endl;
        }
    }

    // Remove bankrupt players from the game
    void removeBankruptPlayers() {
        players.remove_if([](const Player& player) { return player.bankrupt; });
    }
};

// Main function to initiate the game
int main() {
    srand(time(0)); // Seed for random number generation

    Board gameBoard;
    int numPlayers;

    // Welcome message and input number of players
    cout << "Welcome to Monopoly Simplified!" << endl;
    cout << "Enter number of players: ";
    cin >> numPlayers;
    cin.ignore();

    // Add players to the game
    for (int i = 0; i < numPlayers; ++i) {
        string playerName;
        bool isAI = false;
        cout << "Enter name for player " << (i + 1) << ": ";
        getline(cin, playerName);
        cout << "Is this player an AI? (y/n): ";
        char aiChoice;
        cin >> aiChoice;
        cin.ignore();

        if (aiChoice == 'y' || aiChoice == 'Y') {
            isAI = true;
        }

        gameBoard.addPlayer(playerName, isAI);
    }

    // Main game loop
    auto playerIt = gameBoard.players.begin();
    while (true) {
        if (playerIt == gameBoard.players.end()) {
            playerIt = gameBoard.players.begin();
        }

        if (!playerIt->bankrupt) {
            cout << "\n" << playerIt->name << "'s turn:" << endl;
            gameBoard.playTurn(*playerIt);

            // Additional options for human players
            if (!playerIt->isAI && !playerIt->bankrupt) {
                char option;
                cout << "Do you want to (m)ortgage a property, (t)rade a property, or (s)kip? ";
                cin >> option;
                cin.ignore();
                if (option == 'm') {
                    gameBoard.mortgageProperty(*playerIt);
                } else if (option == 't') {
                    gameBoard.tradeProperty(*playerIt);
                }
            }
        }

        // Display the updated board visualization
        gameBoard.displayBoard();

        // Display all players' status
        gameBoard.displayPlayersStatus();

        // Remove bankrupt players from the game
        gameBoard.removeBankruptPlayers();

        // Check if only one player is left
        if (gameBoard.players.size() == 1) {
            cout << "\n" << gameBoard.players.front().name << " is the last player remaining and wins the game!" << endl;
            break;
        }

        // Ask if the game should continue
        char continueGame;
        cout << "Continue playing? (y/n): ";
        cin >> continueGame;
        if (continueGame == 'n') {
            break;
        }

        ++playerIt;
    }

    // End of the game
    cout << "Game Over!" << endl;

    // Display the final board state
    gameBoard.displayBoard();

    // Display each player's final status
    for (const auto& player : gameBoard.players) {
        cout << "\nFinal status for " << player.name << ":" << endl;
        cout << "Money: $" << player.money << endl;
        cout << "Properties Owned: ";
        if (player.propertiesOwned.empty()) {
            cout << "None" << endl;
        } else {
            for (const auto& property : player.propertiesOwned) {
                cout << property << " ";
            }
            cout << endl;
        }
        int totalUpgrades = std::accumulate(player.propertyUpgrades.begin(), player.propertyUpgrades.end(), 0,
            [](int sum, const pair<string, int>& upgrade) {
                return sum + upgrade.second;
            });
        cout << "Total Upgrades: " << totalUpgrades << endl;
        cout << "Bankrupt: " << (player.bankrupt ? "Yes" : "No") << endl;
    }

    // Display total wealth of each player
    gameBoard.displayTotalWealth();

    return 0;
}
