using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using NZBDrive.Model;
using System.Diagnostics;

namespace NZBDrive.View
{
    [ProvideToolboxControl("General", true)]
    public partial class NZBDriveView : UserControl, INotifyPropertyChanged
    {
        private NZBDriveModel _model;

        public NZBDriveModel Model
        {
            get { return _model; }
            set
            {
                if (_model != null) throw new ArgumentException("NZBDrive already set");
                _model = value;
                DataContext = this;
            }
        }

        private NZBFilePartCollection _selectedFileDetails;

        public NewsServerCollection NewsServerCollection { get { return _model.NewsServerCollection; } }
        public NewsServerThrottling NewsServerThrottling { get { return _model.NewsServerThrottling; } }
        public NZBFileList MountedNZBFileList { get { return _model.MountedNZBFileList; } }
        public Log Log { get { return _model.Log; } }
 
        public NZBFilePartCollection SelectedFileDetails 
        { 
            get 
            {
                return _selectedFileDetails; 
            }
            private set
            {
                _selectedFileDetails = value;
                NotifyPropertyChanged();
            }
        }


        public NZBDriveView()
        {
            InitializeComponent();

            NZBFileListView.SelectedNZBFileChanged = (NZBFile file) => 
            { 
                SelectedFileDetails = file!=null ? file.Content : null; 
            };
            NZBFileListView.OpenMountedNZBFile = (NZBFile file) => 
            {
                string mountfolder = _model.Options.DriveLetter + ":\\" + file.FileName;
                Process.Start("explorer.exe", "/root," + mountfolder);
            };

        }

        public event PropertyChangedEventHandler PropertyChanged;

        private void NotifyPropertyChanged([CallerMemberName] String propertyName = "")
        {
            if (PropertyChanged != null)
            {
                PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
            }
        }
    }
}
