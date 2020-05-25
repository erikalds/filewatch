import React from 'react';
import ReactDOM from 'react-dom';
import TreeMenu, { ItemComponent } from 'react-simple-tree-menu';
import './index.css';

class FileContainerView extends React.Component {
    constructor(props) {
        super(props);
    }
    render() {
        return (
            <div className="FileContainerview">
            </div>
        );
    }
}

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
                <TreeMenu
                    data={this.state.treeData}
                    onClickItem={({ key, label, ...props }) => {
                        this.props.viewFile(key);
                    }}
                >
                {({ search, items }) => (
                    <ul>
                        { items.map(({key, ...props}) => (
                            <ItemComponent
                                key={key}
                                label={this.createLabel(props)}
                                {...this.restOfProps(props)} />
                        ))}
                    </ul>
                )}
                </TreeMenu>
            </div>
        );
    }
    createLabel(props) {
        let label = props.label + ", ";
        if (typeof props.size !== 'undefined') {
            label += props.size + " B, ";
        }
        else {
            label += " --, ";
        }
        var time = new Date(props.mtime);
        label += time.toISOString();
        return label;
    }
    restOfProps(props) {
        delete props.label;
        return props;
    }
}

class FilewatcherView extends React.Component {
    constructor(props) {
        super(props);
        this.state = {
            files: [],
            activeFile: null,
        };
    }
    render() {
        console.log("FilewatcherView render state: ", this.state);
        return (
            <div className="FilewatcherView">
                <h1>Filewatcher</h1>
                <Filelist
                    viewFile={(key) => this.viewFile(key)}
                />
                <FileContainerView
                    files={this.state.files}
                    activeFile={this.state.activeFile}
                    setActiveFile={(key) => this.viewFile(key)}
                    closeFile={(key) => this.closeFile(key)}
                />
            </div>
        );
    }
    viewFile(key) {
        var files = this.state.files.slice();
        if (!files.includes(key)) {
            files.push(key);
        }
        this.setState({
            files: files,
            activeFile: key,
        });
    }
    closeFile(key) {
        var files = this.state.files.slice();
        if (!files.includes(key)) {
            return;
        }
        var idx = files.indexOf(key);
        files = files.slice(0, idx).concat(files.slice(idx + 1));
        this.setState({
            files: files,
            activeFile: files[Math.max(0, idx - 1)],
        });

    }
}

ReactDOM.render(
        <FilewatcherView />,
        document.getElementById('root')
);
