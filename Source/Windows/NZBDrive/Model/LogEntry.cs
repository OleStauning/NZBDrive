using System;

namespace NZBDrive.Model
{
    public class LogEntry
    {
        public DateTime Time { get; set; }
        public NZBDriveDLL.LogLevelType LogLevel { get; set; }
        public string Message { get; set; }
    }
}
