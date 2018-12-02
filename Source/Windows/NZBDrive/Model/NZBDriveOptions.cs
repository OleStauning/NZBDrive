using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace NZBDrive.Model
{
    public enum LogLevel : int
    {
        [Description("Debug")]
        Debug = 0,
        [Description("Info")]
        Info = 1,
        [Description("Warning")]
        Warning = 2,
        [Description("Error")]
        Error = 3,
        [Description("Fatal")]
        Fatal = 4
    };

    public class NZBDriveOptionsData
    {
        public LogLevel LogLevel { get; set; }
        public string CachePath { get; set; }
        public char DriveLetter { get; set; }

        public NZBDriveOptionsData()
        { 
        }

        public NZBDriveOptionsData(NZBDriveOptionsData other)
        {
            LogLevel = other.LogLevel;
            CachePath = other.CachePath;
            DriveLetter = other.DriveLetter;
        }
    }

    public class NZBDriveOptions : NZBDriveOptionsData, INotifyPropertyChanged
    {
        private NZBDriveDLL.NZBDrive _nzbDrive;

        public NZBDriveOptions(NZBDriveDLL.NZBDrive nzbDrive)
            : base()
        {
            _nzbDrive = nzbDrive;

            string home = Directory.GetParent(
                Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData)).FullName;
            if (Environment.OSVersion.Version.Major >= 6)
            {
                home = Directory.GetParent(home).ToString();
            }

            CachePath = home+"\\NZBDriveCache";
            DriveLetter = 'N';
            LogLevel = Model.LogLevel.Warning;
        }

        new public LogLevel LogLevel
        {
            get
            {
                return base.LogLevel; 
            }
            set 
            { 
                base.LogLevel = value;
                switch (value)
                {
                    case Model.LogLevel.Debug: _nzbDrive.LogLevel = NZBDriveDLL.LogLevelType.Debug; break;
                    case Model.LogLevel.Info: _nzbDrive.LogLevel = NZBDriveDLL.LogLevelType.Info; break;
                    case Model.LogLevel.Warning: _nzbDrive.LogLevel = NZBDriveDLL.LogLevelType.Warning; break;
                    case Model.LogLevel.Error: _nzbDrive.LogLevel = NZBDriveDLL.LogLevelType.Error; break;
                    case Model.LogLevel.Fatal: _nzbDrive.LogLevel = NZBDriveDLL.LogLevelType.Fatal; break;
                }
                FireChangedEvent();
                OnPropertyChanged();
            }
        }

        public int LogLevelIndex
        {
            get { return (int)LogLevel; }
            set { LogLevel = (LogLevel)value; }
        }
        
        new public char DriveLetter
        {
            get { return base.DriveLetter; }
            set 
            {
                var available = AvailableDriveLetters;
                if (available.Count < 1) return;
                if (value == base.DriveLetter) return;
                if (!available.Contains(value))
                { 
                    // Choose drive-letter after value
                    for (int idx = 0; idx < available.Count; idx++)
                    {
                        if (available[idx] < value)
                        {
                            value = available[idx];
                            break;
                        }
                    }
                    for (int idx = available.Count-1; idx >= 0; idx--)
                    {
                        if (available[idx] > value)
                        {
                            value = available[idx];
                            break;
                        }
                    }

                }
                base.DriveLetter = value;
                _nzbDrive.DriveLetter = value.ToString();
                FireChangedEvent();
                OnPropertyChanged();
            }
        }

        public List<char> AvailableDriveLetters
        {
            get
            {
                List<char> driveLetters = new List<char>(26); // Allocate space for alphabet
                for (int i = (int)'D'; i < (int)'Z'; i++) // increment from ASCII values for A-Z
                {
                    driveLetters.Add(Convert.ToChar(i)); // Add uppercase letters to possible drive letters
                }

                foreach (string drive in Directory.GetLogicalDrives())
                {
                    if (drive[0]!=base.DriveLetter)
                        driveLetters.Remove(drive[0]); // removed used drive letters from possible drive letters
                }

                return driveLetters;
            }
        }

        public void SetValues(NZBDriveOptionsData other)
        {
            LogLevel = other.LogLevel;
            CachePath = other.CachePath;
            DriveLetter = other.DriveLetter;
        }

        public SettingsChangedDelegate SettingsChanged;

        private void FireChangedEvent()
        {
            var handler = SettingsChanged;
            if (handler != null) handler(this);
        }


        public event PropertyChangedEventHandler PropertyChanged;
        protected virtual void OnPropertyChanged([CallerMemberName] string propertyName = "")
        {
            PropertyChangedEventHandler handler = PropertyChanged;
            if (handler != null) handler(this, new PropertyChangedEventArgs(propertyName));
        }

    }
}
