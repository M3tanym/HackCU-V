#ifndef HandSignalCollectionH
#define HandSignalCollectionH

class HandSignalCollection {
    public:
      HandSignalCollection();
      bool save();
      bool add(HandSignal &hs, std::string &name, std::string &command);
      bool remove(int i);
      bool remove(std::string &name);
      std::string getName(int i) const;
      std::string getCommand(int i) const;

      friend class EventListener;
      friend std::ostream &operator<<(std::ostream &os, const HandSignalCollection &hsc);
      friend int main(int argc, const char* argv[]);

    private:
      int getNumFingers(int i) const;
      std::vector<HandSignal> signals;
      std::vector<std::string> names;
      std::vector<std::string> commands;
};

#endif
