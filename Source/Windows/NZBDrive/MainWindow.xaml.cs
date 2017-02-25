using Microsoft.Win32;
using NZBDrive.Model;
using NZBDrive.View;
using NZBDrive.WPFMessageView;
using System;
using System.IO;
using System.IO.Pipes;
using System.Linq;
using System.Text.RegularExpressions;
using System.Windows;

namespace NZBDrive
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public sealed partial class MainWindow : Window, IDisposable
    {
        private NZBDriveDLL.NZBDrive _nzbDrive;
        private NZBDriveModel _model;
        private NamedPipeServerStream _pipe;
        private AsyncCallback _pipeCallback;

        public MainWindow(string[] args)
        {
            InitializeComponent();

            SystemEvents.SessionEnding += SystemEvents_SessionEnding;

            _nzbDrive = new NZBDriveDLL.NZBDrive();

            _nzbDrive.LogLevel = NZBDriveDLL.LogLevelType.Info;

            this.Closing += MainWindow_Closing;

            _nzbDrive.EventLog += _nzbDrive_EventLog;

            _model = new NZBDriveModel(_nzbDrive);
            this.DataContext = _model;
            NZBDriveView.Model = _model;
            StatusBox.Status = _model.Status;
            ToolBar.Model = _model;

            _nzbDrive.ConnectionStateChanged += _nzbDrive_ConnectionStateChanged;
            _nzbDrive.NZBFileOpen += _nzbDrive_NZBFileOpen;
            _nzbDrive.NZBFileClose += _nzbDrive_NZBFileClose;
            _nzbDrive.FileAdded += _nzbDrive_FileAdded;
            _nzbDrive.FileRemoved += _nzbDrive_FileRemoved;
            _nzbDrive.FileInfo += _nzbDrive_FileInfo;
            _nzbDrive.FileSegmentStateChanged += _nzbDrive_FileSegmentStateChanged;
            _nzbDrive.ConnectionInfo += _nzbDrive_ConnectionInfo;
            _model.MountStatusHandle += _model_MountStatus;

            NZBDriveSettings.Load(_model);

            _model.SettingsChanged += model_SettingsChanged;

            this.ContentRendered += MainWindow_ContentRendered;

            _pipe = new NamedPipeServerStream("NZBDriveApplicationPipe", PipeDirection.In,
                254, PipeTransmissionMode.Message, PipeOptions.Asynchronous);

            _pipeCallback = new AsyncCallback(PipedConnectionCallBack);
            _pipe.BeginWaitForConnection(_pipeCallback, this);

            _model.Mount(args);

        }

        void UnhookEvents()
        {            
            _model.SettingsChanged -= model_SettingsChanged;

            _nzbDrive.ConnectionStateChanged -= _nzbDrive_ConnectionStateChanged;
            _nzbDrive.NZBFileOpen -= _nzbDrive_NZBFileOpen;
            _nzbDrive.NZBFileClose -= _nzbDrive_NZBFileClose;
            _nzbDrive.FileAdded -= _nzbDrive_FileAdded;
            _nzbDrive.FileRemoved -= _nzbDrive_FileRemoved;
            _nzbDrive.FileInfo -= _nzbDrive_FileInfo;
            _nzbDrive.FileSegmentStateChanged -= _nzbDrive_FileSegmentStateChanged;
            _nzbDrive.ConnectionInfo -= _nzbDrive_ConnectionInfo;
            _model.MountStatusHandle -= _model_MountStatus;

            _nzbDrive.EventLog -= _nzbDrive_EventLog;
            SystemEvents.SessionEnding -= SystemEvents_SessionEnding;

            _pipe.Close();

        }

        void MainWindow_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            NZBDriveView.Model.StopDrive();
            UnhookEvents();
        }

        void SystemEvents_SessionEnding(object sender, SessionEndingEventArgs e)
        {
            string msg="";

            switch (e.Reason)
            {
                case SessionEndReasons.Logoff: msg = "User is logging off, stopping NZBDrive"; break;
                case SessionEndReasons.SystemShutdown: msg = "System is shutting down, stopping NZBDrive"; break;
            }

            NZBDriveView.Model.Log.Add(new LogEntry() { Time = DateTime.Now, LogLevel = NZBDriveDLL.LogLevelType.Warning, Message = msg });

            NZBDriveView.Model.StopDrive();
            Application.Current.Shutdown();
        }

        private void PipedConnectionCallBack(IAsyncResult result)
        {
            ActivateWindow();
            try
            {
                _pipe.EndWaitForConnection(result); // THROWS EXCEPTION ON PURPOSE ON EXIT..

                using (StreamReader reader = new StreamReader(_pipe))
                {
                    string line;
                    while ((line = reader.ReadLine()) != null)
                    {
                        _model.Mount(line);
                    }
                }
            }
            catch
            {
                // If the pipe is closed before a client ever connects, EndWaitForConnection() will throw an exception.
                // If we are in here that is probably the case so just return.
                return;
            }
            _pipe = new NamedPipeServerStream("NZBDriveApplicationPipe", PipeDirection.In,
                254, PipeTransmissionMode.Message, PipeOptions.Asynchronous);
            _pipe.BeginWaitForConnection(new AsyncCallback(PipedConnectionCallBack), this);
        }

        private void ActivateWindow()
        {
            this.Dispatcher.BeginInvoke((Action)(() =>
            {
               Activate();
            }));
        }

        private void Window_Drop(object sender, DragEventArgs e)
        {
            if (e.Data.GetDataPresent(DataFormats.FileDrop))
            {
                // Note that you can have more than one file.
                string[] files = (string[])e.Data.GetData(DataFormats.FileDrop);

                foreach (var file in files)
                {
                    _model.Mount(file);
                }
            }
        }

        bool _shown = false;
        void MainWindow_ContentRendered(object sender, EventArgs e)
        {
            if (!_shown)
            {
                SuggestUsenetProvider();
                CleanCacheDir();
                ShowAddUsenetServer();
                _shown = true;
            }
        }

        private void ShowAddUsenetServer()
        { 
            if (_model.NewsServerCollection.Count()==0)
            {
                ToolBar.AddUsenetServer();
            }
        }

        private void SuggestUsenetProvider()
        {
            if (_model.NewsServerCollection.Count == 0)
            {
                var dialog = new SuggestUsenetProviderWindow();
                dialog.Owner = Application.Current.MainWindow;


                if (dialog.ShowDialog().HasValue)
                {
                    if (dialog.MessageView.Result == WPFMessageView.WPFMessageViewResult.Yes)
                    {
                        System.Diagnostics.Process.Start("http://www.nzbdrive.com/signup");                    
                    }
                }
            }
        }

        private void CleanCacheDir()
        {
            if (Directory.Exists(_model.Options.CachePath))
            {
                try
                {
                    var files = Directory.GetFiles(_model.Options.CachePath)
                        .Where(name => Regex.IsMatch(name, @"\\[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$",
                            RegexOptions.IgnoreCase));

                    if (files.Count() > 0)
                    {
                        var dialog = new ClearCachePathWindow();
                        dialog.Owner = this;
                        var res = dialog.ShowDialog();

                        if (res.HasValue && res.Value && dialog.MessageView.Result == WPFMessageViewResult.Yes)
                        {
                            foreach (string file in files) File.Delete(file);
                        }
                    }
                }
                catch (System.Exception)
                {
                    MessageBox.Show(Application.Current.MainWindow,
                        string.Format("Could not access cache path {0}", _model.Options.CachePath)
                        );
                }
            }
        }
        
        public string LicenseKey { get { return _model.LicenseKey; } }

        public Version CurrentVersion
        {
            get { return _model.CurrentVersion; }
            set { _model.CurrentVersion = value; }
        }

        public Version LatestVersion
        {
            get { return _model.LatestVersion; }
            set { _model.LatestVersion = value; }
        }

        public DateTime LatestVersionCheckDate
        {
            get { return _model.LatestVersionCheckDate; }
            set { _model.LatestVersionCheckDate = value; }
        }

        void _model_MountStatus(int nzbID, int parts, int total)
        {
            this.Dispatcher.BeginInvoke((Action)(() =>
            {
                try
                {
                    NZBDriveView.Model.MountStatus(nzbID, parts, total);
                }
                catch (Exception e)
                {
                    MessageBox.Show("_model_MountStatusFunction Failed: " + e.Message);
                }
            }));
        }

        void _nzbDrive_FileSegmentStateChanged(int fileID, int segment, NZBDriveDLL.SegmentState state)
        {
            this.Dispatcher.BeginInvoke((Action)(() =>
            {
                try
                {
                    NZBDriveView.Model.FileSegmentStateChangedEvent(fileID, segment, state);
                }
                catch (Exception e)
                {
                    MessageBox.Show("_nzbDrive_FileSegmentStateChanged Failed: " + e.Message);
                }
            }));
        }

        void _nzbDrive_FileInfo(int fileID, string name, ulong size)
        {
            this.Dispatcher.BeginInvoke((Action)(() =>
            {
                try
                {
                    NZBDriveView.Model.FileInfoEvent(fileID,name,size);
                }
                catch (Exception e)
                {
                    MessageBox.Show("_nzbDrive_FileInfo Failed: " + e.Message);
                }
            }));
        }

        void _nzbDrive_FileRemoved(int fileID)
        {
            this.Dispatcher.BeginInvoke((Action)(() =>
            {
                try
                {
                    NZBDriveView.Model.FileRemovedEvent(fileID);
                }
                catch (Exception e)
                {
                    MessageBox.Show("_nzbDrive_FileRemoved Failed: " + e.Message);
                }
            }));
        }

        void _nzbDrive_FileAdded(int nzbID, int fileID, int segments, ulong size)
        {
            this.Dispatcher.BeginInvoke((Action)(() =>
            {
                try
                {
                    NZBDriveView.Model.FileAddedEvent(nzbID, fileID, segments, (ulong)size);
                }
                catch (Exception e)
                {
                    MessageBox.Show("_nzbDrive_FileAdded Failed: " + e.Message);
                }
            }));
        }

        void _nzbDrive_NZBFileClose(int nzbID)
        {
            this.Dispatcher.BeginInvoke((Action)(() =>
            {
                try
                {
                    NZBDriveView.Model.NZBFileCloseEvent(nzbID);
                }
                catch (Exception e)
                {
                    MessageBox.Show("_nzbDrive_NZBFileClose Failed: " + e.Message);
                }
            }));
        }

        void _nzbDrive_NZBFileOpen(int nzbID, string path)
        {
            this.Dispatcher.BeginInvoke((Action)(() =>
            {
                try
                {
                    NZBDriveView.Model.NZBFileOpenEvent(nzbID, path);
                }
                catch (Exception e)
                {
                    MessageBox.Show("_nzbDrive_NZBFileOpen Failed: " + e.Message);
                }
            }));
        }

        void _nzbDrive_ConnectionStateChanged(NZBDriveDLL.ConnectionState state, int server, int thread)
        {
            this.Dispatcher.BeginInvoke((Action)(() =>
            {
                try
                {
                    NZBDriveView.Model.NewsConnectionEvent(state,server,thread);
                }
                catch (Exception e)
                {
                    MessageBox.Show("_nzbDrive_OnNewsConnectionEvent Failed: " + e.Message);
                }
            }));
        }

        void _nzbDrive_EventLog(NZBDriveDLL.LogLevelType lvl, string msg)
        {
            this.Dispatcher.BeginInvoke((Action)(() => 
            {
                try
                {
                    NZBDriveView.Model.Log.Add(new LogEntry() { Time = DateTime.Now, LogLevel = lvl, Message = msg });
                    if (lvl == NZBDriveDLL.LogLevelType.PopupError)
                    {
                        var dialog = new ErrorWindow(msg);
                        dialog.Owner = Window.GetWindow(this);
                        dialog.ShowDialog();
//                            MessageBox.Show(this, msg, "NZBDrive Error", MessageBoxButton.OK, MessageBoxImage.Error, MessageBoxResult.OK);
                    }
                    else if (lvl == NZBDriveDLL.LogLevelType.PopupInfo)
                    {
                        var dialog = new InfoWindow(msg);
                        dialog.Owner = Window.GetWindow(this);
                        dialog.ShowDialog();
                        //                        MessageBox.Show(this, msg, "NZBDrive Info", MessageBoxButton.OK, MessageBoxImage.Information, MessageBoxResult.OK);
                        if (msg.StartsWith("Trial license timeout"))
                        {
                            Application.Current.Shutdown();
                        }
                    }
                }
                catch (ObjectDisposedException)
                { 
                    // Can happen when application is shutting down.
                }
                catch (InvalidOperationException)
                {
                    // Can happen when application is shutting down.
                }
                catch (Exception e)
                {
                    MessageBox.Show("_nzbDrive_OnEventLog Failed: " + e.Message);
                }
            }));
        }

        void _nzbDrive_ConnectionInfo(long time, int server, ulong bytesTX, ulong bytesRX, uint missingSegmentCount, uint connectionTimeoutCount, uint errorCount)
        {
            this.Dispatcher.BeginInvoke((Action)(() =>
            {
                try
                {
                    NZBDriveView.Model.NewsServerCollection.ServerStateUpdated(time, server,bytesTX,bytesRX, missingSegmentCount, connectionTimeoutCount, errorCount);
                    NZBDriveView.Model.Status.ServerStateUpdated(time, server, bytesTX, bytesRX, missingSegmentCount, connectionTimeoutCount, errorCount);
                }
                catch (Exception e)
                {
                    MessageBox.Show("_nzbDrive_ServerStateUpdated Failed: " + e.Message);
                }
            }));
        }

        void model_SettingsChanged(object sender)
        {
            NZBDriveSettings.Save(_model);
        }


        private bool _disposed = false;
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
                    UnhookEvents();
                    _nzbDrive.Dispose();
                }
                _disposed = true;
            }
        }
    }
}
