using System;
using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace NZBDrive.Model
{
    public class NewsServerConnectionData: INotifyPropertyChanged
    {
        private NewsServerConnectionData() { }
        public NewsServerConnectionData(NewsServerConnectionData other)
        {
            SetValues(other);
        }
        public void SetValues(NewsServerConnectionData other)
        {
            this.ServerName = other.ServerName;
            this.UserName = other.UserName;
            this.Password = other.Password;
            this.Port = other.Port;
            this.Connections = other.Connections;
            this.Encryption = other.Encryption;
        }
        private string _serverName;
        public string ServerName 
        {
            get { return _serverName; }
            set { _serverName = value; OnPropertyChanged("ValidHostname"); } 
        }
        public string UserName { get; set; }
        public string Password { get; set; }
        private string _port;
        public string Port 
        {
            get { return _port; }
            set { _port = value; OnPropertyChanged("ValidHostname"); }
        }
        public int Connections { get; set; }
        public NewsServerEncryption Encryption { get; set; }
        public int EncryptionIndex
        {
            get
            {
                return (int)Encryption;
            }
            set
            {
                Encryption = (NewsServerEncryption)value;
            }
        }

        public bool ValidHostname
        {
            get { return Uri.CheckHostName(ServerName) != UriHostNameType.Unknown && Port.Length>0; }
        }

        static public NewsServerConnectionData NewDefaultConnection 
        {
            get { 
                return new NewsServerConnectionData() 
                { 
                    ServerName = "", 
                    UserName = "", 
                    Password = "", 
                    Connections = 4, 
                    Port = "119", 
                    Encryption = NewsServerEncryption.None 
                };  
            } 
        }

        public event PropertyChangedEventHandler PropertyChanged;
        protected virtual void OnPropertyChanged([CallerMemberName] string propertyName = "")
        {
            PropertyChangedEventHandler handler = PropertyChanged;
            if (handler != null) handler(this, new PropertyChangedEventArgs(propertyName));
        }

    }

    public class NewsServerConnection : NewsServerConnectionData
    {
        NZBDriveDLL.NZBDrive _nzbDrive;
        int _handle = -1;

        public NewsServerConnection()
            : base(NewsServerConnectionData.NewDefaultConnection)
        {
            _nzbDrive = null;
        }

        public NewsServerConnection(NZBDriveDLL.NZBDrive nzbDrive)
            : base(NewsServerConnectionData.NewDefaultConnection)
        {
            _nzbDrive = nzbDrive;
        }

        public NewsServerConnection(NZBDriveDLL.NZBDrive nzbDrive, NewsServerConnectionData other)
            : base(other)
        {
            _nzbDrive = nzbDrive;
        }

        public NewsServerConnection(NewsServerConnection other)
            : base(other)
        {
            _nzbDrive = other._nzbDrive;
        }

        internal int DoDisconnect()
        {
            _nzbDrive.RemoveServer(_handle);
            return _handle;
        }

        internal int DoConnect()
        {
            return _handle = _nzbDrive.AddServer(new NZBDriveDLL.UsenetServer()
            { 
                ServerName = ServerName,
                UserName = UserName,
                Password = Password,
                Port = Port,
                Connections = Connections,
                Pipeline = 3,
                Priority = 0,
                Timeout = 10,
                UseSSL = Encryption == NewsServerEncryption.SSL ? true : false
            });
        }

    }
}
