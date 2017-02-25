using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Windows.Controls;
using System.Windows.Media;

namespace NZBDrive.View
{
    [ProvideToolboxControl("General", true)]
    public partial class ColorVector : UserControl, INotifyPropertyChanged
    {

        public ObservableCollection<SolidColorBrush> Squares { get; private set; }

        public ColorVector()
        {
            InitializeComponent();

            Squares = new ObservableCollection<SolidColorBrush>();

            DataContext = this;
        }

        public int Count
        {
            get { return Squares.Count; }
            set
            {
                while (Squares.Count != value)
                {
                    switch (Squares.Count < value)
                    {
                        case true: Squares.Add(_defaultSquareColor); break;
                        case false: Squares.RemoveAt(0); break;
                    }
                }
            }
        }
    
        public event PropertyChangedEventHandler PropertyChanged;
        private void RaisePropertyChanged(string propertyName)
        {
            if (PropertyChanged != null)
            {
                PropertyChanged(this, 
                    new PropertyChangedEventArgs(propertyName));
            }
        }

        SolidColorBrush _defaultSquareColor = new SolidColorBrush(Colors.White);

        public SolidColorBrush DefaultSquareColor
        {
            get { return _defaultSquareColor; }
            set
            {
                _defaultSquareColor = value;
                for (int i = 0; i < Squares.Count; ++i) Squares[i] = _defaultSquareColor;
                RaisePropertyChanged("DefaultSquareColor");
            }
        }

    }
}
