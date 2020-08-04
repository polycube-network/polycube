import groovy.json.JsonOutput

def slackNotification(buildStatus,String notification_url = "",project_name = "") {
    if (buildStatus == 'UNSTABLE') {
        title = "${BRANCH_NAME} - BUILD UNSTABLE on ${project_name}"
        text = "Build n°${BUILD_NUMBER} of branch ${BRANCH_NAME} unstable (${BUILD_URL})"
        colorCode = '#FFFF00'
    } else if (buildStatus == 'SUCCESS') {
        text = "Build n°${BUILD_NUMBER} of branch ${BRANCH_NAME} successful (${BUILD_URL})"
        title = "${BRANCH_NAME} - BUILD SUCCESS on ${project_name}"
        colorCode = '#00FF00'
    } else if (buildStatus == 'STARTED') {
        colorCode = '#D4DADF'
        title = "${BRANCH_NAME} - BUILD STARTED on ${project_name}"
        text = "Build n°${BUILD_NUMBER} of branch ${BRANCH_NAME} started (${BUILD_URL})"
    } else {
        echo "The build status is: ${buildStatus}"
        title = "${BRANCH_NAME} - BUILD FAILED on ${project_name}"
        text = "Build n°${BUILD_NUMBER} of branch ${BRANCH_NAME} failed with status ${buildStatus} (${BUILD_URL})"
        colorCode = '#FF0000'
    }

    def payload= JsonOutput.toJson(
        [
            attachments: [
                [
                    color: "${colorCode}",
                    text: "${text}",
                    title: "${title}"
                ]
            ]
        ]
    )
    sh "curl -X POST --fail -H 'Content-type: application/json' --data '${payload}' ${notification_url}"
}

/**
* Trigger Github PR Check ( can be : error, failure, pending, or success)
**/
def setGithubStatus(commit_id, build_status, github_token, String project_name, String feature, String message){
    sh """
        curl -k --fail -H "Authorization: token ${github_token}" -H "Content-Type: application/json" -H "Accept: application/vnd.github.v3+json" -X POST \
        -d '{
            "state": "${build_status}",
            "target_url": "${BUILD_URL}",
            "description": "${message}",
            "context": "${feature}"
        }' \
        https://api.github.com/repos/${project_name}/statuses/${commit_id} \
    """
}

def commit_id

pipeline {
    agent {
        label 'docker'
    }
    triggers {
        cron( env.BRANCH_NAME == 'master' ? '0 2 * * *' : '' )
    }
    options {
        disableConcurrentBuilds()
        timeout(time: 4, unit: 'HOURS')
        buildDiscarder(logRotator(numToKeepStr: '30'))
    }
    environment {
        image_tag = "${env.GIT_COMMIT}"
        image_name = "polycube"
        container = "polycubed"
        registryCred = credentials('polycube-repo')
        notification_url = credentials('slack-notifications')
        github_token = credentials('github_status_update')
    }
    stages {
        stage ('Init Notification Handlers') {
            agent {
              label "docker"
            }
            steps {
                script {
                    commit_id = sh returnStdout: true, script: "git rev-parse HEAD"
                    setGithubStatus("${commit_id}", "pending", github_token, "polycube-network/polycube", "Same Instance Tests", "Same Instance Tests")
                    setGithubStatus("${commit_id}", "pending", github_token, "polycube-network/polycube", "Iptables Tests", "Iptables Tests")
                    setGithubStatus("${commit_id}", "pending", github_token, "polycube-network/polycube", "Clean Instance Tests", "Clean Instance Tests")
                    setGithubStatus("${commit_id}", "pending", github_token, "polycube-network/polycube", "Docker Image Build", "Docker Image Build")
                    slackNotification("STARTED",notification_url,"Polycube")
                }
            }
        }
        stage ('Clean Directory and Checkout') {
            steps {
                dir('.') {
                    deleteDir()
                }
                checkout scm
            }
        }
        stage('Parallel Docker Build') {
            parallel {
                stage("Build Docker Mode: default") {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            docker.withRegistry("", 'polycube-repo') {
                                sh """
                                    export DOCKER_BUILDKIT=1
                                    docker system prune --all --force
                                    # checking if the latest pushed image has the same tag of the current image_tag
                                    if wget -qO- https://hub.docker.com/v2/repositories/polycubebot/${image_name}-default/tags/ | jq -r '.["results"][]["name"]' | grep -q ${image_tag}
                                    then
                                        echo "image ${image_name}-default already pushed with tag ${image_tag}"
                                        echo "skipping image building"
                                    else
                                        docker build . -t "polycubebot/${image_name}-default:${image_tag}" --build-arg DEFAULT_MODE=default
                                        docker push polycubebot/${image_name}-default:${image_tag}
                                    fi
                                """
                            }
                        }
                    }
                }
                stage("Build Docker Mode: pcn-k8s") {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            docker.withRegistry("", 'polycube-repo') {
                                sh """
                                    export DOCKER_BUILDKIT=1
                                    docker system prune --all --force
                                    # checking if the latest pushed image has the same tag of the current image_tag
                                    if wget -qO- https://hub.docker.com/v2/repositories/polycubebot/${image_name}-pcn-k8s/tags/ | jq -r '.["results"][]["name"]' | grep -q ${image_tag}
                                    then
                                        echo "image ${image_name}-pcn-k8s already pushed with tag ${image_tag}"
                                        echo "skipping image building"
                                    else
                                        docker build . -t "polycubebot/${image_name}-pcn-k8s:${image_tag}" --build-arg DEFAULT_MODE=pcn-k8s
                                        docker push polycubebot/${image_name}-pcn-k8s:${image_tag}
                                    fi
                                """
                            }
                        }
                    }
                }
                stage("Build Docker Mode: pcn-iptables") {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            docker.withRegistry("", 'polycube-repo') {
                                sh """
                                    export DOCKER_BUILDKIT=1
                                    docker system prune --all --force
                                    # checking if the latest pushed image has the same tag of the current image_tag
                                    if wget -qO- https://hub.docker.com/v2/repositories/polycubebot/${image_name}-pcn-iptables/tags/ | jq -r '.["results"][]["name"]' | grep -q ${image_tag}
                                    then
                                        echo "image ${image_name}-pcn-iptables already pushed with tag ${image_tag}"
                                        echo "skipping image building"
                                    else
                                        docker build . -t "polycubebot/${image_name}-pcn-iptables:${image_tag}" --build-arg DEFAULT_MODE=pcn-iptables
                                        docker push polycubebot/${image_name}-pcn-iptables:${image_tag}
                                    fi
                                """
                            }
                        }
                    }
                }
            }
            post {
                failure {
                    script {
                        setGithubStatus("${commit_id}", "failure", github_token, "polycube-network/polycube", "Docker Image Build", "Docker Image Build")
                    }
                }
                success {
                    script {
                        setGithubStatus("${commit_id}", "success", github_token, "polycube-network/polycube", "Docker Image Build", "Docker Image Build")
                    }
                }
            }
        }
        stage('Testing') {
            parallel {
                stage('Launch Clean Instance Tests') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            docker.withRegistry("", 'polycube-repo') {
                                sh """
                                    docker stop $container || true
                                    docker system prune --all --force
                                    set +e

                                    # Overload environment variables in order to adapt the behavior of run_tests script
                                    export KILL_COMMAND="docker stop $container"
                                    
                                    # /proc is mounted under /host/proc to have a complete visibility of the namespaces created outside of the container
                                    export polycubed="docker run -d --name $container --rm --privileged --pid=host --cap-add ALL --network host -v /proc:/host/proc -v /lib/modules:/lib/modules:ro -v /var/run/netns/:/var/run/netns:rw -v /usr/src:/usr/src:ro -v /etc/localtime:/etc/localtime:ro polycubebot/${image_name}-default:${image_tag}"

                                    # Launch the polycubed daemon
                                    \$polycubed /bin/sleep infinity

                                    # Copy binaries, headers and libraries from docker to host fs
                                    sudo ./CI/extract_from_docker_image.sh $container

                                    \$KILL_COMMAND

                                    \$polycubed
                                    docker ps

                                    cd tests/
                                    # For the moment, tests always returns 0 to enable junit generation
                                    ./run-tests.sh || true
                                    export LC_ALL=C
                                    virtualenv venv -p python3
                                    . venv/bin/activate
                                    pip install -r converter/requirements.txt
                                    export TEST_RESULTS=`ls -1 test_results_*`
                                    python3 converter/to_junit.py CleanInstance
                                    cd ..

                                    # Cleaning slave
                                    sudo ./CI/clean_slave.sh
                                """
                            }
                            }
                    }
                    post {
                        always {
                            dir(".") {
                                junit allowEmptyResults: true, testResults: "tests/output.xml"
                                archiveArtifacts artifacts: 'tests/test_results*'
                            }
                            script {
                                sh """
                                    docker stop $container || true
                                    docker system prune --all --force
                                """
                            }
                            sh 'rm -rf ${WORKSPACE}/*'
                        }
                        failure {
                            script {
                                setGithubStatus("${commit_id}", "error", github_token, "polycube-network/polycube", "Clean Instance Tests", "Clean Instance Tests")
                            }
                        }
                        unstable {
                            script {
                                setGithubStatus("${commit_id}", "failure", github_token, "polycube-network/polycube", "Clean Instance Tests", "Clean Instance Tests")
                            }
                        }
                        success {
                            script {
                                setGithubStatus("${commit_id}", "success", github_token, "polycube-network/polycube", "Clean Instance Tests", "Clean Instance Tests")
                            }
                        }
                    }
                }
                stage('Launch Same Instance Tests') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            docker.withRegistry("", 'polycube-repo') {
                                sh """
                                    docker stop $container || true
                                    docker system prune --all --force
                                    set +e

                                    # Overload environment variables in order to adapt the behavior of run_tests script
                                    export KILL_COMMAND="docker stop $container"

                                    # /proc is mounted under /host/proc to have a complete visibility of the namespaces created outside of the container
                                    export polycubed="docker run -d --name $container --rm --privileged --pid=host --cap-add ALL --network host -v /proc:/host/proc -v /lib/modules:/lib/modules:ro -v /var/run/netns/:/var/run/netns:rw -v /usr/src:/usr/src:ro -v /etc/localtime:/etc/localtime:ro polycubebot/${image_name}-default:${image_tag}"

                                    # Launch the polycubed daemon
                                    \$polycubed /bin/sleep infinity

                                    # Copy binaries, headers and libraries from docker to host fs
                                    sudo ./CI/extract_from_docker_image.sh $container

                                    \$KILL_COMMAND

                                    \$polycubed

                                    docker ps
                                    cd tests/ && ./run-tests.sh false || true
                                    export LC_ALL=C
                                    virtualenv venv -p python3
                                    . venv/bin/activate
                                    pip install -r converter/requirements.txt
                                    export TEST_RESULTS=`ls -1 test_results_*`
                                    python3 converter/to_junit.py SameInstance
                                    cd ..
                                    
                                    # Cleaning slave
                                    sudo ./CI/clean_slave.sh
                                """
                            }
                        }
                    }
                    post {
                        always {
                            script {
                                dir(".") {
                                    junit allowEmptyResults: true, testResults: "tests/output.xml"
                                    archiveArtifacts artifacts: 'tests/test_results*'
                                }
                                sh """
                                    docker stop $container || true
                                    docker system prune --all --force
                                """
                            }
                            sh 'rm -rf ${WORKSPACE}/*'
                        }
                        failure {
                            script {
                                setGithubStatus("${commit_id}", "error", github_token, "polycube-network/polycube", "Same Instance Tests", "Same Instance Tests")
                            }
                        }
                        unstable {
                            script {
                                setGithubStatus("${commit_id}", "failure", github_token, "polycube-network/polycube", "Same Instance Tests", "Same Instance Tests")
                            }
                        }
                        success {
                            script {
                                setGithubStatus("${commit_id}", "success", github_token, "polycube-network/polycube", "Same Instance Tests", "Same Instance Tests")
                            }
                        }
                    }
                }
                stage('Launch Iptables Tests') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            docker.withRegistry("", 'polycube-repo') {
                                sh """
                                    docker stop $container || true
                                    docker system prune --all --force
                                    set +e

                                    # Overload environment variables in order to adapt the behavior of run_tests script
                                    export KILL_COMMAND="docker stop $container"

                                    # /proc is mounted under /host/proc to have a complete visibility of the namespaces created outside of the container
                                    export polycubed="docker run -d --name $container --rm --privileged --pid=host --cap-add ALL --network host -v /proc:/host/proc -v /lib/modules:/lib/modules:ro -v /var/run/netns/:/var/run/netns:rw -v /usr/src:/usr/src:ro -v /etc/localtime:/etc/localtime:ro polycubebot/${image_name}-pcn-iptables:${image_tag}"

                                    # Launch the polycubed daemon
                                    \$polycubed /bin/sleep infinity

                                    # Copy binaries, headers and libraries from docker to host fs
                                    sudo ./CI/extract_from_docker_image.sh $container

                                    \$KILL_COMMAND

                                    \$polycubed
                                    
                                    cd tests/ && ./run-tests-iptables.sh || true
                                    export LC_ALL=C
                                    virtualenv venv -p python3
                                    . venv/bin/activate
                                    pip install -r converter/requirements.txt
                                    export TEST_RESULTS=`ls -1 test_results_*`
                                    python3 converter/to_junit.py Iptables
                                    cd ..

                                    # Cleaning slave
                                    sudo ./CI/clean_slave.sh
                                """
                            }
                        }
                    }
                    post {
                        always {
                            dir(".") {
                                junit allowEmptyResults: true, testResults: "tests/output.xml"
                                archiveArtifacts artifacts: 'tests/test_results*'
                            }
                            script {
                                sh """
                                    docker stop $container || true
                                    docker system prune --all --force
                                """
                            }
                            sh 'rm -rf ${WORKSPACE}/*'
                        }
                        failure {
                            script {
                                setGithubStatus("${commit_id}", "error", github_token, "polycube-network/polycube", "Iptables Tests", "Iptables Tests")
                            }
                        }
                        unstable {
                            script {
                                setGithubStatus("${commit_id}", "failure", github_token, "polycube-network/polycube", "Iptables Tests", "Iptables Tests")
                            }
                        }
                        success {
                            script {
                                setGithubStatus("${commit_id}", "success", github_token, "polycube-network/polycube", "Iptables Tests", "Iptables Tests")
                            }
                        }
                    }
                }
            }
        }
        stage("Promote Images") {
            when {
                branch 'master'
            }
            agent {
                label "docker"
            }
            steps {
                script {
                    docker.withRegistry("", 'polycube-repo') {
                        sh """
                            export DOCKER_BUILDKIT=1
                            docker pull "polycubebot/${image_name}-default:${image_tag}"
                            docker tag "polycubebot/${image_name}-default:${image_tag}" "polycubenetwork/polycube:latest"
                            docker push "polycubenetwork/polycube:latest"
                            docker pull "polycubebot/${image_name}-pcn-k8s:${image_tag}"
                            docker tag "polycubebot/${image_name}-pcn-k8s:${image_tag}" "polycubenetwork/k8s-pod-network:latest"
                            docker push "polycubenetwork/k8s-pod-network:latest"
                            docker pull "polycubebot/${image_name}-pcn-iptables:${image_tag}"
                            docker tag "polycubebot/${image_name}-pcn-iptables:${image_tag}" "polycubenetwork/polycube-pcn-iptables:latest"
                            docker push "polycubenetwork/${image_name}-pcn-iptables:latest"
                            docker system prune --all --force
                        """
                    }
                }
            }
        }
        stage("Release version from TAG") {
            when {
               buildingTag()	
            }
            agent {
                label "docker"
            }
            steps {
                script {
                    var tagName = "${env.TAG_NAME}"
                    docker.withRegistry("", 'polycube-repo') {
                        sh """
                            export DOCKER_BUILDKIT=1
                            docker pull "polycubebot/${image_name}-default:${image_tag}"
                            docker tag "polycubebot/${image_name}-default:${image_tag}" "polycubenetwork/polycube:${env.TAG_NAME}"
                            docker push "polycubenetwork/polycube:${env.TAG_NAME}"
                            docker pull "polycubebot/${image_name}-pcn-k8s:${image_tag}"
                            docker tag "polycubebot/${image_name}-pcn-k8s:${image_tag}" "polycubenetwork/k8s-pod-network:${env.TAG_NAME}"
                            docker push "polycubenetwork/k8s-pod-network:${env.TAG_NAME}"
                            docker pull "polycubebot/${image_name}-pcn-iptables:${image_tag}"
                            docker tag "polycubebot/${image_name}-pcn-iptables:${image_tag}" "polycubenetwork/polycube-pcn-iptables:${env.TAG_NAME}"
                            docker push "polycubenetwork/${image_name}-pcn-iptables:${env.TAG_NAME}"
                            docker system prune --all --force
                        """
                    }
                }
            }
        }
    }
    post {
        always {
            slackNotification(currentBuild.result,notification_url,"Polycube")
            sh 'rm -rf ${WORKSPACE}/*'
        }
    }
}