using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;

namespace NZBDrive.Model
{
    public delegate void SettingsChangedDelegate(object sender);

    public sealed class NZBDriveModel : INotifyPropertyChanged, IDisposable
    {
        private NZBDriveDLL.NZBDrive _nzbDrive;

        public Log Log { get; private set; }
        public NZBDriveStatus Status { get; private set; }
        public NewsServerCollection NewsServerCollection { get; private set; }
        public NewsServerThrottling NewsServerThrottling { get; private set; }
        public NZBFileList MountedNZBFileList { get; private set; }
        public NZBDriveOptions Options { get; private set; }

        public NZBDriveDLL.MountStatusFunction MountStatusHandle;
        public event SettingsChangedDelegate SettingsChanged;

        public string Title
        {
            get { return "NZBDrive " + _currentVersion.ToString(3) + " (" + Options.DriveLetter + ":\\)"; }
        }

        private void FireChangedEvent(object sender = null)
        {
            var handler = SettingsChanged;
            if (handler != null) handler(sender??this);
            OnPropertyChanged("Title");
        }

        public void StartDrive()
        {
            _nzbDrive.Start();
        }

        public void StopDrive()
        {
            _nzbDrive.Stop();
        }

        public NZBDriveModel(NZBDriveDLL.NZBDrive nzbDrive)
        {
            Log = new Log(nzbDrive);
            _nzbDrive = nzbDrive;
            Status = new NZBDriveStatus(nzbDrive);
            NewsServerCollection = new NewsServerCollection(nzbDrive);
            NewsServerThrottling = new NewsServerThrottling(nzbDrive);
            MountedNZBFileList = new NZBFileList(nzbDrive);
            Options = new NZBDriveOptions(nzbDrive);
            SetEvents();
        }

        private void SetEvents()
        {
            NewsServerCollection.SettingsChanged += FireChangedEvent;
            NewsServerThrottling.SettingsChanged += FireChangedEvent;
            Options.SettingsChanged += FireChangedEvent;
        }

        private void UnsetEvents()
        {
            NewsServerCollection.SettingsChanged -= FireChangedEvent;
            NewsServerThrottling.SettingsChanged -= FireChangedEvent;
            Options.SettingsChanged -= FireChangedEvent;
        }

        public void Mount(string filename)
        {
            _nzbDrive.Mount(filename, MountStatusHandle);
        }

        internal void Mount(string[] args)
        {
            foreach (var filename in args) _nzbDrive.Mount(filename, MountStatusHandle);
        }

        public bool NewVersionAvailable
        {
            get { return _currentVersion < _latestVersion; }
        }

        private Version _currentVersion;

        public Version CurrentVersion
        {
            get { return _currentVersion; }
            set { _currentVersion = value; FireChangedEvent(this); FireChangedEvent("NewVersionAvailable"); }
        }

        public String CurrentVersionString
        {
            get { return _currentVersion.ToString(3); }
        }

        private Version _latestVersion;

        public Version LatestVersion
        {
            get { return _latestVersion; }
            set { _latestVersion = value; FireChangedEvent(this); FireChangedEvent("NewVersionAvailable"); }
        }

        private DateTime _latestVersionCheckDate;

        public DateTime LatestVersionCheckDate 
        {
            get { return _latestVersionCheckDate; }
            set { _latestVersionCheckDate = value; FireChangedEvent(this); }
        }

        internal void MountStatus(int nzbID, int parts, int total)
        {
            MountedNZBFileList.MountStatus(nzbID, parts, total);
        }

        internal void FileSegmentStateChangedEvent(int fileID, int segment, NZBDriveDLL.SegmentState state)
        {
            MountedNZBFileList.FileSegmentStateChangedEvent(fileID, segment, state);
        }

        internal void FileInfoEvent(int fileID, string name, ulong size)
        {
            MountedNZBFileList.FileInfoEvent(fileID, name, size);
        }

        internal void FileRemovedEvent(int fileID)
        {
            MountedNZBFileList.FileRemovedEvent(fileID);
        }

        internal void NZBFileOpenEvent(int nzbID, string path)
        {
            MountedNZBFileList.NZBFileOpenEvent(nzbID, path);
        }

        internal void NZBFileCloseEvent(int nzbID)
        {
            MountedNZBFileList.NZBFileCloseEvent(nzbID);
        }

        internal void FileAddedEvent(int nzbID, int fileID, int segments, ulong p)
        {
            MountedNZBFileList.FileAddedEvent(nzbID, fileID, segments, p);
        }

        internal void NewsConnectionEvent(NZBDriveDLL.ConnectionState state, int server, int thread)
        {
            NewsServerCollection.NewsConnectionEvent(state, server, thread);
        }

        public event PropertyChangedEventHandler PropertyChanged;

        private void OnPropertyChanged([CallerMemberName] string propertyName = "")
        {
            PropertyChangedEventHandler handler = PropertyChanged;
            if (handler != null) handler(this, new PropertyChangedEventArgs(propertyName));
        }

        private bool _disposed=false;

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }
        private void Dispose(bool disposing)
        {
            if (!_disposed)
            {
                if (disposing)
                {
                    UnsetEvents();
                }
                _disposed = true;
            }
        }
    }
}
