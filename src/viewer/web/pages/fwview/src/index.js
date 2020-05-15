import React from 'react';
import ReactDOM from 'react-dom';
import './index.css';

class FilewatcherView extends React.Component {
    render() {
        return (
            <div className="FilewatcherView">
                <h1>Hello, World!</h1>
            </div>
        );
    }
}

ReactDOM.render(
        <FilewatcherView />,
        document.getElementById('root')
);
