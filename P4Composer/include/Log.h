#ifndef LOG_H
#define LOG_H

#define LOG_LEVEL_NONE 0
#define LOG_LEVEL_SHORT 1
#define LOG_LEVEL_ALL 2

class Log{
    public:
        Log(const Log&) = delete;
        void operator=(const Log&) = delete;

        static void SetOutputDir(const std::string& outputDir);
        static void SetLogLevel(const int logLevel);

        static Log& Get();

        static void Reset();
        static void Push();
        static void Pop();

        static void MSG(const std::string& msg, const int LOG_LEVEL);

        /*
        Close file and reopen in append mode.
        */
        static void Save();

        static bool isEnable();
        static void Enable(const bool enable);
        static void EnablePush(const bool enable);
        static void EnablePop();
    private:
        Log();
        ~Log();

        FILE *m_fp;
        std::vector<int> m_stack;
        std::vector<bool> m_enableStack;
        std::string m_outputDir;
        bool enable;

        int m_logLevel;
};

#endif