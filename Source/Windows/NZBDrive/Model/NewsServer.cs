using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;

namespace NZBDrive.Model
{
    public class NewsServer : INotifyPropertyChanged
    {
        public string DisplayServer
        {
            get
            {
                return "nntp" + (ServerConnection.Encryption == NewsServerEncryption.None ? "" : "s") + "://" + ServerConnection.UserName + "@" + ServerConnection.ServerName + ":" + ServerConnection.Port;
            }
        }
        private NewsServerConnection _serverConnection;
        public NewsServerConnection ServerConnection 
        {
            get { return _serverConnection; }
            set 
            {
                _serverConnection = value; 
                OnPropertyChanged(); 
                OnPropertyChanged("DisplayServer");
            }
        }
        private NewsServerStatus _serverStatus;
        public NewsServerStatus ServerStatus 
        {
            get { return _serverStatus; }
            set { _serverStatus = value; OnPropertyChanged(); }
        }

//        private NewsServerConnectionData server;

        public NewsServer(NZBDriveDLL.NZBDrive nzbDrive, NewsServerConnectionData server)
        {
            ServerConnection = new NewsServerConnection(nzbDrive, server);
            ServerStatus = new NewsServerStatus()
            {
                ConnectionDataRate = 0,
                ConnectionDataRateUnit = DataRateUnit.Unlimited,
                ErrorCount = 0,
                ConnectionStatusCollection = new ObservableCollection<NewsServerConnectionStatus>(Enumerable.Repeat(NewsServerConnectionStatus.Disconnected,server.Connections))
            };
        }

        public event PropertyChangedEventHandler PropertyChanged;
        protected virtual void OnPropertyChanged([CallerMemberName] string propertyName = "")
        {
            PropertyChangedEventHandler handler = PropertyChanged;
            if (handler != null) handler(this, new PropertyChangedEventArgs(propertyName));
        }

    }
}
