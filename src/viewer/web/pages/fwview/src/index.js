import React from 'react';
import ReactDOM from 'react-dom';
import TreeMenu from 'react-simple-tree-menu';
import './index.css';

class Filelist extends React.Component {
    constructor(props) {
        super(props);
        this.state = {
            treeData: {},
        };

        fetch("/v1.0/files").then(response => {
            if (!response.ok)
                throw Error("response was bad:", response.status, response.statusText);

            response.json().then(data => {
                this.setState({
                    treeData: data,
                });
            });
        });
    }
    render() {
        return (
            <div className="Filelist">
                <TreeMenu data={this.state.treeData} />
            </div>
        );
    }
}

class FilewatcherView extends React.Component {
    render() {
        return (
            <div className="FilewatcherView">
                <h1>Filewatcher</h1>
                <Filelist/>
            </div>
        );
    }
}

ReactDOM.render(
        <FilewatcherView />,
        document.getElementById('root')
);
