using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;

namespace NZBDrive.Model
{
    public sealed class NewsServerCollection : ObservableCollection<NewsServer>, IDisposable
    {
        private NZBDriveDLL.NZBDrive _nzbDrive;
        private Dictionary<int, NewsServerStatus> _statusMap = new Dictionary<int,NewsServerStatus>();

        public NewsServerCollection(NZBDriveDLL.NZBDrive nzbDrive)
            : base()
        {
            _nzbDrive = nzbDrive;
            SetEvents();
        }

        public NewsServerCollection(NZBDriveDLL.NZBDrive nzbDrive, IEnumerable<NewsServer> value)
            : base(value)
        {
            _nzbDrive = nzbDrive;
            SetEvents();
        }

        private void SetEvents()
        {
            CollectionChanged += NewsServerCollection_CollectionChanged;
        }

        private void UnsetEvents()
        {
            CollectionChanged -= NewsServerCollection_CollectionChanged;
        }

        public event SettingsChangedDelegate SettingsChanged;

        private void FireChangedEvent()
        {
            var handler = SettingsChanged;
            if (handler != null) handler(this);
        }

        void NewsServerCollection_CollectionChanged(object sender, System.Collections.Specialized.NotifyCollectionChangedEventArgs e)
        {
            bool changes = false;

            // TODO: Handle change server more elegant...

            if (
                e.Action == NotifyCollectionChangedAction.Remove || 
                e.Action == NotifyCollectionChangedAction.Replace || 
                e.Action == NotifyCollectionChangedAction.Reset)
            {
                foreach (NewsServer server in e.OldItems)
                {
                    int serverID = server.ServerConnection.DoDisconnect();
                    _statusMap.Remove(serverID);
                    changes = true;
                }

            }

            if (e.Action == NotifyCollectionChangedAction.Add || 
                e.Action == NotifyCollectionChangedAction.Replace)
            {
                foreach (NewsServer server in e.NewItems)
                {
                    int serverID = server.ServerConnection.DoConnect();
                    _statusMap[serverID] = server.ServerStatus;
                    changes = true;
                }
            }

            if (changes) FireChangedEvent();
        }

        internal void Add(NewsServerConnectionData server)
        {
            this.Add(new NewsServer(_nzbDrive, server));
            FireChangedEvent();
        }
        internal void Change(int idx, NewsServerConnectionData server)
        {
            this[idx] = new NewsServer(_nzbDrive, server);
            FireChangedEvent();
        }
        internal void Remove(int idx)
        {
            this.RemoveAt(idx);
            FireChangedEvent();
        }


        internal void NewsConnectionEvent(NZBDriveDLL.ConnectionState state, int server, int thread)
        {
            NewsServerStatus newsServerStatus;

            if (_statusMap.TryGetValue(server, out newsServerStatus))
            {
                newsServerStatus.NewsConnectionEvent(state, thread);
            }
        }

        internal void ServerStateUpdated(long time, int server, ulong bytesTX, ulong bytesRX, uint missingSegmentCount, uint connectionTimeoutCount, uint errorCount)
        {
            NewsServerStatus newsServerStatus;

            if (_statusMap.TryGetValue(server,out newsServerStatus))
            {
                newsServerStatus.ServerStateUpdated(time, bytesTX, bytesRX, missingSegmentCount, connectionTimeoutCount, errorCount);
            }
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
