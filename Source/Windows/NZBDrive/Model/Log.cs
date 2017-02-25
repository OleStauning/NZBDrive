using System.Collections.Generic;
using System.Collections.ObjectModel;

namespace NZBDrive.Model
{
    public class Log : ObservableCollection<LogEntry> 
    {
        private NZBDriveDLL.NZBDrive _nzbDrive;

        public Log(NZBDriveDLL.NZBDrive nzbDrive)
            : base()
        {
            _nzbDrive = nzbDrive;
        }

        public Log(NZBDriveDLL.NZBDrive nzbDrive, IEnumerable<LogEntry> value)
            : base(value)
        {
            _nzbDrive = nzbDrive;
        }
    }
}
